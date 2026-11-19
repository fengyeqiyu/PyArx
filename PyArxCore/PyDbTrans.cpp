// PyDbTrans.cpp — 参考 MapleArxLib DbTrans 简化版
#include "stdafx.h"
#include "logPath.h"
#include "PyDbTrans.h"
#include "stdafx.h"
#include "PyDbEntity.h"
#include "stdafx.h"
#include "PyDbObject.h"
#include "PyErrorStatusException.h"
#include "stdafx.h"
#include "PyDbBlockTableRecord.h"
#include "stdafx.h"
#include "PyDbDatabase.h"
#include "stdafx.h"
#include "PyDbTypeCaster.h"
#include "stdafx.h"
#include <memory>
#include "stdafx.h"
#include <stdexcept>
#include "stdafx.h"
#include <unordered_map>

// 全局类注册表：AcRxClass* → Python 类对象（openObject 自动类型推导用）
static std::unordered_map<AcRxClass*, py::object> g_classMap;

// ===== 诊断日志（idle 卡死定位）：追加写 <PyArxCore.pyd 目录>\log\pydbtrans_diag.log =====
static void pdtDiag(const char* step) {
	FILE* pf = nullptr;
	if (_wfopen_s(&pf, pyarxLogFile(L"pydbtrans_diag.log", L"PyArxCore.pyd").c_str(), L"ab") != 0 || !pf) return;
	// 每条带 GetTickCount64 毫秒时间戳 + 步骤名
	fprintf(pf, "[%llu] %s\n", (unsigned long long)GetTickCount64(), step);
	fflush(pf);
	fclose(pf);
}

void registerPyClass(AcRxClass* acRxClass, py::object pyClass) {
	g_classMap[acRxClass] = pyClass;
}

// 当前文档构造（GUI 专属）：锁文档 + 启动事务
PyDbTrans::PyDbTrans() {
#ifdef PYARX_GUI
	pdtDiag("CTOR_GUI enter");
	pDoc = acDocManager->curDocument();
	pdtDiag(pDoc ? "CTOR_GUI curDocument OK" : "CTOR_GUI curDocument=NULL");
	if (!pDoc) throw std::runtime_error("无当前文档（无头模式请用 PyDbTrans(db)）");

	// 无条件尝试 lockDocument：按钮回调（modeless 面板消息）中 myLockMode 可能为 kWrite，
	// 但 acad 实际空闲未持写锁；此前按 myLockMode 跳过加锁，导致 getObject(kForWrite)
	// 返回 eLockViolation(m_es=320)、currentSpace=nullptr、实体无法写入。
	// 无条件 lockDocument：
	//   - 命令上下文（当前线程真持锁）→ 返回 eLockViolation → 视为已锁(eOk)，直接写库；
	//   - modeless 回调 / Python 顶层（未持锁）→ 返回 eOk → 加锁成功，正常写库。
	AcAp::DocLockMode lockMode = pDoc->myLockMode();
	{
		char buf[128];
		sprintf_s(buf, "CTOR_GUI myLockMode=%d", (int)lockMode);
		pdtDiag(buf);
	}
	pdtDiag("CTOR_GUI lockDocument BEFORE (always)");
	m_es = acDocManager->lockDocument(pDoc, AcAp::kWrite);
	{
		char buf[128];
		sprintf_s(buf, "CTOR_GUI lockDocument AFTER m_es=%d", (int)m_es);
		pdtDiag(buf);
	}
	if (m_es == Acad::eOk) {
		m_locked = true;
	} else if (m_es == Acad::eLockViolation) {
		// 当前线程/命令已持写锁属预期，不是错误；恢复 eOk，
		// 否则 Commit 的 IsOk() 分支会误走 abortTransaction，回滚本事务内 add 的实体。
		m_es = Acad::eOk;
		pdtDiag("CTOR_GUI lockDocument = eLockViolation -> treat as already locked");
	}
	// 其它锁失败：保持 m_es 错误值，Commit 走 abort（合理）。

	pDb = pDoc->database();
	pdtDiag("CTOR_GUI pDb=database() done");
	{
		const ACHAR* pTitle = pDoc->docTitle();
		std::string ns;
		if (pTitle) {
			int len = ::WideCharToMultiByte(CP_UTF8, 0, pTitle, -1, nullptr, 0, nullptr, nullptr);
			ns.resize(len > 0 ? len - 1 : 0);
			if (len > 0) ::WideCharToMultiByte(CP_UTF8, 0, pTitle, -1, &ns[0], len, nullptr, nullptr);
		}
		char buf[384];
		sprintf_s(buf, "CTOR_GUI docTitle=%s", ns.c_str());
		pdtDiag(buf);
	}
#else
	throw std::runtime_error("当前构建为无头模式，请用 PyDbTrans(db) 绑定独立数据库");
#endif
	pTm = pDb->transactionManager();
	{
		char buf[128];
		sprintf_s(buf, "CTOR_GUI pTm=%p", (void*)pTm);
		pdtDiag(buf);
	}
	if (!pTm) throw std::runtime_error("事务管理器不可用");
	pdtDiag("CTOR_GUI startTransaction BEFORE");
	pTrans = pTm->startTransaction();
	{
		char buf[128];
		sprintf_s(buf, "CTOR_GUI startTransaction AFTER pTrans=%p", (void*)pTrans);
		pdtDiag(buf);
	}
	m_committed = false;
}

// 独立数据库构造（无头）：跳过文档锁
PyDbTrans::PyDbTrans(AcDbDatabase* pDbArg) {
	if (!pDbArg) throw std::runtime_error("无效的数据库");
	pDb = pDbArg;
	pTm = pDb->transactionManager();
	if (!pTm) throw std::runtime_error("事务管理器不可用");
	pTrans = pTm->startTransaction();
	m_standalone = true;
	pDoc = nullptr;
	m_committed = false;
}

// 析构兜底提交（参考 ~DbTrans() { Commit(); }）
PyDbTrans::~PyDbTrans() {
	commit();
}

// 提交事务 + 解锁（幂等）
void PyDbTrans::commit() {
	if (m_committed) return;
	m_committed = true;
	pdtDiag("COMMIT enter");

	if (pTrans) {
		if (IsOk()) {
			pdtDiag("COMMIT endTransaction BEFORE");
			pTm->endTransaction();
			pdtDiag("COMMIT endTransaction AFTER");
		} else {
			{
				char buf[128];
				sprintf_s(buf, "COMMIT abortTransaction BEFORE m_es=%d", (int)m_es);
				pdtDiag(buf);
			}
			pTm->abortTransaction();
			pdtDiag("COMMIT abortTransaction AFTER");
		}
		pTrans = nullptr;
	}

	if (m_locked && pDoc) {
		pdtDiag("COMMIT unlockDocument BEFORE");
		acDocManager->unlockDocument(pDoc);
		pdtDiag("COMMIT unlockDocument AFTER");
	}
	m_locked = false;
	pdtDiag("COMMIT exit");
}

// 中止事务 + 解锁（幂等）
void PyDbTrans::abort() {
	if (m_committed) return;
	m_committed = true;

	if (pTrans) {
		pTm->abortTransaction();
		pTrans = nullptr;
	}

	if (m_locked && pDoc) {
		acDocManager->unlockDocument(pDoc);
	}
	m_locked = false;
}

template <class T>
T* PyDbTrans::getObject(AcDbObjectId objid, AcDb::OpenMode mode, bool openErased) {
	if (!pTrans) return nullptr;
	T* pObj = nullptr;
	{
		char buf[128];
		sprintf_s(buf, "GETOBJECT objId=%llu mode=%d BEFORE", (unsigned long long)objid.asOldId(), (int)mode);
		pdtDiag(buf);
	}
	m_es = pTrans->getObject<T>(pObj, objid, mode, openErased);
	{
		char buf[128];
		sprintf_s(buf, "GETOBJECT AFTER m_es=%d pObj=%p", (int)m_es, (void*)pObj);
		pdtDiag(buf);
	}
	return IsOk() ? pObj : nullptr;
}

// 当前空间块表记录：显式用 ModelSpace（按钮绘制应落在模型空间），
// 避免 currentSpaceId() 在异步命令上下文返回 PaperSpace 或错误空间。
AcDbBlockTableRecord* PyDbTrans::currentSpace() {
	AcDbObjectId msId;
	AcDbBlockTable* pBt = nullptr;
	if (pDb->getSymbolTable(pBt, AcDb::kForRead) != Acad::eOk) {
		pdtDiag("CURRENTSPACE getSymbolTable FAILED");
		return nullptr;
	}
	pBt->getAt(ACDB_MODEL_SPACE, msId);
	pBt->close();
	AcDbObjectId csId = pDb->currentSpaceId();
	{
		char buf[160];
		sprintf_s(buf, "CURRENTSPACE modelSpaceId=%llu currentSpaceId=%llu",
			(unsigned long long)msId.asOldId(), (unsigned long long)csId.asOldId());
		pdtDiag(buf);
	}
	return getObject<AcDbBlockTableRecord>(msId, AcDb::kForWrite);
}

// Python 可调用：事务内按 ObjectId 打开对象，自动类型推导
py::object PyDbTrans::openObject(AcDbObjectId id, int mode, bool openErased) {
	if (!pTrans) return py::none();
	if (id.isNull()) {
		m_es = Acad::eNullObjectPointer;
		PyWarnIfError(m_es, L"PyDbTrans::openObject(null objectId)");
		return py::none();
	}
	if (mode < 0 || mode > 2) {
		m_es = Acad::eInvalidInput;
		PyWarnIfError(m_es, L"PyDbTrans::openObject(mode must be 0/1/2)");
		return py::none();
	}
	AcDbObject* p = nullptr;
	try {
		m_es = pTrans->getObject(p, id, static_cast<AcDb::OpenMode>(mode), openErased);
	} catch (...) {
		m_es = Acad::eNullObjectPointer;
		PyWarnIfError(m_es, L"PyDbTrans::openObject(illegal objectId)");
		return py::none();
	}
	if (!IsOk() || !p) return py::none();

	// 类型还原：先按精确类查，查不到就沿 AcRxClass 继承链向上找。
	// 只按精确类查的后果：块定义内部的 INSERT/ATTDEF 等对象落到基类 PyDbObject，
	// Python 侧拿不到 attributeIds/openAttribute，属性块文字永远读不出来。
	for (AcRxClass* pc = p->isA(); pc; pc = pc->myParent()) {
		auto hit = g_classMap.find(pc);
		if (hit == g_classMap.end())
			continue;
		try {
			py::object obj = hit->second.attr("cast")(PyDbObject(p));
			if (!obj.is_none())
				return obj;
		} catch (...) {
			// 该层的 Python 类 not registered for this pointer type，继续向上
		}
	}
	return py::cast(PyDbObject(p));
}

// 追加实体到指定块表记录（参考 addEntity(btr, ent)）
bool PyDbTrans::addEntity(AcDbBlockTableRecord* btr, PyDbEntity& ent) {
	pdtDiag("ADDENTITY(btr,ent) enter");
	if (!btr || ent.isNull()) { pdtDiag("ADDENTITY(btr,ent) btr/ent null"); return false; }

	pdtDiag("ADDENTITY appendAcDbEntity BEFORE");
	// 防呆：块表记录未以写模式打开时 appendAcDbEntity 会触发 ARX 致命断言（无头进程直接崩）
	if (!btr->isWriteEnabled()) {
		pdtDiag("ADDENTITY btr not write-enabled, refused");
		PyWarnIfError(Acad::eNotOpenForWrite, L"PyDbTrans::addEntity(btr 未以写模式打开，请先 openObject(id,1))");
		return false;
	}
	m_es = btr->appendAcDbEntity(ent.impObj());
	{
		char buf[128];
		sprintf_s(buf, "ADDENTITY appendAcDbEntity AFTER m_es=%d", (int)m_es);
		pdtDiag(buf);
	}
	if (!IsOk()) return false;

	pdtDiag("ADDENTITY addNewlyCreatedDBRObject BEFORE");
	m_es = pTm->addNewlyCreatedDBRObject(ent.impObj(), Adesk::kTrue);
	{
		char buf[128];
		sprintf_s(buf, "ADDENTITY addNewlyCreatedDBRObject AFTER m_es=%d", (int)m_es);
		pdtDiag(buf);
	}
	if (!IsOk()) {
		ent.impObj()->close();
		ent.detachDbObject();
		return false;
	}
	pdtDiag("ADDENTITY(btr,ent) OK");
	return true;
}

// 便捷版：追加到当前空间
bool PyDbTrans::addEntity(PyDbEntity& ent) {
	pdtDiag("ADDENTITY(ent) enter");
	AcDbBlockTableRecord* btr = currentSpace();
	{
		char buf[128];
		sprintf_s(buf, "ADDENTITY(ent) currentSpace=%p", (void*)btr);
		pdtDiag(buf);
	}
	return btr ? addEntity(btr, ent) : false;
}

// 新建对象入事务：SDK 2024 里是 AcDbTransactionManager::addNewlyCreatedDBRObject(返回 ErrorStatus)，
// add=true 时事务提交后由 DB 接管所有权
bool PyDbTrans::addNewlyCreatedDBObject(PyDbObject& obj, bool add) {
	if (!pTm || !obj.impObj()) return false;
	if (!PyWarnIfError(pTm->addNewlyCreatedDBRObject(obj.impObj(), add), L"PyDbTrans::addNewlyCreatedDBObject")) return false;
	return true;
}

// 排队图形刷新（GUI 下新实体立即可见）
void PyDbTrans::flushScreen() {
	pdtDiag("FLUSHSCREEN enter");
	if (pTm) pTm->queueForGraphicsFlush();
	pdtDiag("FLUSHSCREEN exit");
}

// Python with 支持
PyDbTrans* PyDbTrans::enter() { return this; }

void PyDbTrans::exit(py::object excType, py::object, py::object) {
	// 根因修复：with 块内 Python 异常向上传播时必须 abort 事务回滚，
	// 旧实现无条件 commit，导致异常路径下已追加到当前空间的实体被持久化。
	if (!excType.is_none()) {
		abort();
		return;
	}
	commit();
}

void bind_PyDbTrans(py::module& m) {
	py::class_<PyDbTrans>(m, "PyDbTrans")
		.def(py::init([]() { return new PyDbTrans(); }))
		.def(py::init([](PyDbDatabase& db) {
			return new PyDbTrans(static_cast<AcDbDatabase*>(db.impObj()));
		}), py::arg("db"), "按指定数据库创建事务（独立库，跳过文档锁）")
		.def("openObject", &PyDbTrans::openObject,
			py::arg("object_id"), py::arg("mode") = 1, py::arg("open_erased") = false,
			"事务内按 ObjectId 打开对象；mode 0=读 1=写 2=通知")
		.def("addEntity", py::overload_cast<PyDbEntity&>(&PyDbTrans::addEntity),
			py::arg("entity"), "追加实体到当前空间")
		.def("addEntity", py::overload_cast<AcDbBlockTableRecord*, PyDbEntity&>(&PyDbTrans::addEntity),
			py::arg("block_table_record"), py::arg("entity"), "追加实体到指定块表记录")
		.def("addNewlyCreatedDBObject", &PyDbTrans::addNewlyCreatedDBObject,
			py::arg("obj"), py::arg("add") = true,
			"把新建的表记录/对象登记进当前事务（建块定义、建图层后必须调用）")
		.def("currentSpace", [](PyDbTrans& t) {
			AcDbBlockTableRecord* btr = t.currentSpace();
			return btr ? PyDbBlockTableRecord(btr) : PyDbBlockTableRecord(nullptr);
		}, "返回当前空间块表记录")
		.def("commit", &PyDbTrans::commit, "提交事务并解锁文档")
		.def("abort", &PyDbTrans::abort, "中止事务并解锁文档")
		.def("flushScreen", &PyDbTrans::flushScreen, "排队图形屏幕刷新")
		.def("__enter__", &PyDbTrans::enter, py::return_value_policy::reference)
		.def("__exit__", &PyDbTrans::exit);
}
