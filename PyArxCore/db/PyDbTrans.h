#pragma once
#include "stdafx.h"
#include <acdb.h>
#include <acdocman.h>
#include <dbtrans.h>
#include <dbid.h>
#include <dbsymtb.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;

class PyDbEntity;
class PyDbObject;
class PyDbDatabase;
class AcDbLine;

void registerPyClass(AcRxClass* acRxClass, py::object pyClass);
// 按 AcRxClass 继承链把 AcDbObject 还原成最贴近的 Python 包装（保证派生 API 可用）
py::object pyarxWrapDbObjectByClass(AcDbObject* p);

// 事务 RAII 助手（参考 MapleArxLib DbTrans）：
// 构造即锁文档 + 启动事务，析构自动 commit。
// Python 侧可用 with（__enter__/__exit__），也可手动 new 后调 commit()。
class PyDbTrans {
public:
	Acad::ErrorStatus m_es = Acad::eOk;
	bool m_committed = false;
	bool m_locked = false;
	bool m_standalone = false;

	AcApDocument* pDoc = nullptr;
	AcDbDatabase* pDb = nullptr;
	AcDbTransactionManager* pTm = nullptr;
	AcTransaction* pTrans = nullptr;

	// 当前文档（GUI）
	PyDbTrans();
	// 绑定独立数据库（无头/noDocument，跳过文档锁）
	explicit PyDbTrans(AcDbDatabase* pDbArg);
	~PyDbTrans();

	bool IsOk() const { return m_es == Acad::eOk; }

	// 提交事务 + 解锁文档（幂等）
	void commit();
	// 中止事务 + 解锁文档（幂等）
	void abort();

	template <class T = AcDbEntity>
	T* getObject(AcDbObjectId objid, AcDb::OpenMode mode = AcDb::kForWrite, bool openErased = false);

	py::object openObject(AcDbObjectId objId, int mode = 1, bool openErased = false);

	AcDbBlockTableRecord* currentSpace();
	bool addEntity(AcDbBlockTableRecord* btr, PyDbEntity& ent);
	bool addEntity(PyDbEntity& ent);
	// 新建记录/对象入事务（SDK AcDbTransaction::addNewlyCreatedDBObject）：
	// 缺这条 Python 侧建块定义、建图层会在 add 之后取不到对象
	bool addNewlyCreatedDBObject(PyDbObject& obj, bool add);
	void flushScreen();

	// Python with 支持：enter 返回自身，exit 调 commit
	PyDbTrans* enter();
	void exit(py::object, py::object, py::object);
};

void bind_PyDbTrans(py::module& m);
