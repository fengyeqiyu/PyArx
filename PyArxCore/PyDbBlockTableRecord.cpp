#include "stdafx.h"
#include "dbapserv.h"   // acdbOpenObject（裸打开）+ AcDbTransactionManager
#include "PyErrorStatusException.h"
#include "PyUtils.h"
#include "PyDbBlockTableRecord.h"
#include "PyDbBlockTableRecordIterator.h"
#include "PyDbOpenCache.h"

PyWrapper_Cpp(DbBlockTableRecord);

AcString PyDbBlockTableRecord::comments() const { if (!impObj()) return AcString(); AcString s; if(!PyWarnIfError(impObj()->comments(s), L"AcDbBlockTableRecord::comments")) return AcString(); return s; }
bool  PyDbBlockTableRecord::setComments(const AcString& comments) { if (!impObj()) return false; return PyWarnIfError(impObj()->setComments(comments.kwszPtr()), L"AcDbBlockTableRecord::setComments"); }
AcString PyDbBlockTableRecord::pathName() const { if (!impObj()) return AcString(); AcString s; if(!PyWarnIfError(impObj()->pathName(s), L"AcDbBlockTableRecord::pathName")) return AcString(); return s; }
bool  PyDbBlockTableRecord::setPathName(const AcString& path) { if (!impObj()) return false; return PyWarnIfError(impObj()->setPathName(path.kwszPtr()), L"AcDbBlockTableRecord::setPathName"); }
AcGePoint3d PyDbBlockTableRecord::origin() const { if (!impObj()) return AcGePoint3d(); return impObj()->origin(); }
bool  PyDbBlockTableRecord::setOrigin(const AcGePoint3d& pt) { if (!impObj()) return false; return PyWarnIfError(impObj()->setOrigin(pt), L"AcDbBlockTableRecord::setOrigin"); }
bool PyDbBlockTableRecord::hasAttributeDefinitions() const { if (!impObj()) return false; return impObj()->hasAttributeDefinitions(); }
bool PyDbBlockTableRecord::hasPreviewIcon() const { if (!impObj()) return false; return impObj()->hasPreviewIcon(); }
bool PyDbBlockTableRecord::isAnonymous() const { if (!impObj()) return false; return impObj()->isAnonymous(); }
bool PyDbBlockTableRecord::isFromExternalReference() const { if (!impObj()) return false; return impObj()->isFromExternalReference(); }
bool PyDbBlockTableRecord::isFromOverlayReference() const { if (!impObj()) return false; return impObj()->isFromOverlayReference(); }
bool  PyDbBlockTableRecord::setIsFromOverlayReference(bool bIsOverlay) { if (!impObj()) return false; return PyWarnIfError(impObj()->setIsFromOverlayReference(bIsOverlay), L"AcDbBlockTableRecord::setIsFromOverlayReference"); }
bool PyDbBlockTableRecord::isLayout() const { if (!impObj()) return false; return impObj()->isLayout(); }
uint64_t PyDbBlockTableRecord::getLayoutId() const { if (!impObj()) return 0ULL; return static_cast<uint64_t>(impObj()->getLayoutId().asOldId()); }
bool  PyDbBlockTableRecord::setLayoutId(uint64_t layoutId) {
	if (!impObj()) return false;
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(layoutId));
	return PyWarnIfError(impObj()->setLayoutId(id), L"AcDbBlockTableRecord::setLayoutId");
}
bool PyDbBlockTableRecord::isUnloaded() const { if (!impObj()) return false; return impObj()->isUnloaded(); }
bool  PyDbBlockTableRecord::setIsUnloaded(bool isUnloaded) { if (!impObj()) return false; return PyWarnIfError(impObj()->setIsUnloaded(isUnloaded), L"AcDbBlockTableRecord::setIsUnloaded"); }
int PyDbBlockTableRecord::xrefStatus() const { if (!impObj()) return 0; return static_cast<int>(impObj()->xrefStatus()); }

py::list PyDbBlockTableRecord::getBlockReferenceIds(bool directOnly, bool forceValidity) const {
	if (!impObj()) return py::list();
	AcDbObjectIdArray ids;
	if(!PyWarnIfError(impObj()->getBlockReferenceIds(ids, directOnly, forceValidity), L"AcDbBlockTableRecord::getBlockReferenceIds")) return py::list();
	return py::cast(ids);
}

py::list PyDbBlockTableRecord::getErasedBlockReferenceIds() const {
	if (!impObj()) return py::list();
	AcDbObjectIdArray ids;
	if(!PyWarnIfError(impObj()->getErasedBlockReferenceIds(ids), L"AcDbBlockTableRecord::getErasedBlockReferenceIds")) return py::list();
	return py::cast(ids);
}

bool  PyDbBlockTableRecord::assumeOwnershipOf(const AcDbObjectIdArray& entitiesToMove) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->assumeOwnershipOf(entitiesToMove), L"AcDbBlockTableRecord::assumeOwnershipOf");
}

int PyDbBlockTableRecord::blockScaling() const { if (!impObj()) return 0; return static_cast<int>(impObj()->blockScaling()); }
bool  PyDbBlockTableRecord::setBlockScaling(int blockScaling) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setBlockScaling(static_cast<AcDbBlockTableRecord::BlockScaling>(blockScaling)), L"AcDbBlockTableRecord::setBlockScaling");
}
bool PyDbBlockTableRecord::explodable() const { if (!impObj()) return false; return impObj()->explodable(); }
bool  PyDbBlockTableRecord::setExplodable(bool bExplodable) { if (!impObj()) return false; return PyWarnIfError(impObj()->setExplodable(bExplodable), L"AcDbBlockTableRecord::setExplodable"); }
int PyDbBlockTableRecord::blockInsertUnits() const { if (!impObj()) return 0; return static_cast<int>(impObj()->blockInsertUnits()); }
bool  PyDbBlockTableRecord::setBlockInsertUnits(int insunits) { if (!impObj()) return false; return PyWarnIfError(impObj()->setBlockInsertUnits(static_cast<AcDb::UnitsValue>(insunits)), L"AcDbBlockTableRecord::setBlockInsertUnits"); }
int PyDbBlockTableRecord::postProcessAnnotativeBTR(bool bqueryOnly, bool bScale) {
	if (!impObj()) return 0;
	int stripCnt = 0;
	if(!PyWarnIfError(impObj()->postProcessAnnotativeBTR(stripCnt, bqueryOnly, bScale), L"AcDbBlockTableRecord::postProcessAnnotativeBTR")) return 0;
	return stripCnt;
}
bool  PyDbBlockTableRecord::addAnnoScalestoBlkRefs(bool bScale) { if (!impObj()) return false; return PyWarnIfError(impObj()->addAnnoScalestoBlkRefs(bScale), L"AcDbBlockTableRecord::addAnnoScalestoBlkRefs"); }

bool  PyDbBlockTableRecord::appendEntity(PyDbEntity& pEntity) {
	if (!impObj()) return false;
	// 经典 ARX 写法：块记录以 kForWrite 打开时直接 appendAcDbEntity，随后关闭实体。
	// 旧实现用 startTransaction + addNewlyCreatedDBRObject + detach + endTransaction 五步，
	// 每一步都返回成功，但函数返回后 pybind 仍报 "Caught an unknown exception"，实体也没进库
	// （2026-11-17 实测：同序列包 try/catch 捕获不到，异常在帧外抛出；改经典写法后
	// numEntities 3→4、objectId 可取回、entityIds 能看到新对象）。
	AcDbObjectId outId;
	Acad::ErrorStatus es = impObj()->appendAcDbEntity(outId, pEntity.impObj());
	if (es != Acad::eOk) { PyWarnIfError(es, L"AcDbBlockTableRecord::appendEntity"); return false; }
	pEntity.impObj()->close();   // 入库后必须关闭，否则对象保持打开态，后续 setLayer 等报 eWasOpenForWrite
	pEntity.detachDbObject();    // 包装不再持有对象，避免 Python 侧析构 delete 已入库实体
	return !outId.isNull();
}

uint64_t PyDbBlockTableRecord::appendAcDbEntityId(PyDbEntity& pEntity) {
	if (!impObj()) return 0ULL;
	AcDbObjectId outId;
	Acad::ErrorStatus es = impObj()->appendAcDbEntity(outId, pEntity.impObj());
	if (es != Acad::eOk) { PyWarnIfError(es, L"AcDbBlockTableRecord::appendAcDbEntityId"); return 0ULL; }
	pEntity.impObj()->close();
	pEntity.detachDbObject();
	return outId.isNull() ? 0ULL : static_cast<uint64_t>(outId.asOldId());
}

// 块内实体数量（不含已删除）：直接数 iterator，避免依赖 AcDbBlockTableRecord 内部计数
int PyDbBlockTableRecord::numEntities() const {
	if (!impObj()) return 0;
	AcDbBlockTableRecordIterator* it = nullptr;
	if (impObj()->newIterator(it) != Acad::eOk || !it) return 0;
	int n = 0;
	for (it->start(true, true); !it->done(); it->step(true, true)) {
		AcDbObjectId eid;
		if (it->getEntityId(eid) == Acad::eOk && !eid.isNull()) ++n;
	}
	delete it;
	return n;
}

// 按 ObjectId(oldId) 打开块内实体；mode 0=只读 1=写（默认写，符合图元操作习惯）
PyDbEntity PyDbBlockTableRecord::getEntityById(uint64_t id, int mode) const {
	if (!id) return PyDbEntity(nullptr);
	AcDbObjectId oid;
	oid.setFromOldId(static_cast<Adesk::IntDbId>(id));
	AcDbEntity* pEnt = nullptr;
	Acad::ErrorStatus es = acdbOpenObject(pEnt, oid, static_cast<AcDb::OpenMode>(mode), false);
	if ((es != Acad::eOk || !pEnt) && (es == Acad::eWasOpenForRead || es == Acad::eWasOpenForWrite)) {
		// 该对象已被别处打开：复用打开缓存里的指针，必要时 upgradeOpen 升级到写
		Acad::ErrorStatus es2 = Acad::eOk;
		AcDbObject* pObj = PyDbOpenCache::Get(oid, static_cast<AcDb::OpenMode>(mode), es2);
		pEnt = (es2 == Acad::eOk && pObj) ? AcDbEntity::cast(pObj) : nullptr;
	}
	if (!pEnt) return PyDbEntity(nullptr);
	PyDbOpenCache::Track(pEnt, static_cast<AcDb::OpenMode>(mode));
	return PyDbEntity(pEnt);
}

// openEntityById：与 getEntityById 同一套打开/缓存逻辑，但交出所有权
PyDbEntity* PyDbBlockTableRecord::openEntityById(uint64_t id, int mode) const {
	if (!id) return nullptr;
	AcDbObjectId oid;
	oid.setFromOldId(static_cast<Adesk::IntDbId>(id));
	AcDbEntity* pEnt = nullptr;
	Acad::ErrorStatus es = acdbOpenObject(pEnt, oid, static_cast<AcDb::OpenMode>(mode), false);
	if ((es != Acad::eOk || !pEnt) && (es == Acad::eWasOpenForRead || es == Acad::eWasOpenForWrite)) {
		Acad::ErrorStatus es2 = Acad::eOk;
		AcDbObject* pObj = PyDbOpenCache::Get(oid, static_cast<AcDb::OpenMode>(mode), es2);
		pEnt = (es2 == Acad::eOk && pObj) ? AcDbEntity::cast(pObj) : nullptr;
	}
	if (!pEnt) return nullptr;
	PyDbOpenCache::Track(pEnt, static_cast<AcDb::OpenMode>(mode));
	return new PyDbEntity(pEnt);
}

// 块定义内 ATTDEF 数据：openObject 只能拿到基类，Python 侧 cast 不到 PyDbAttributeDefinition，
// 故在 C++ 内 AcDbAttributeDefinition::cast 后导出常用字段
py::list PyDbBlockTableRecord::attdefData() const {
	py::list out;
	if (!impObj()) return out;
	AcDbBlockTableRecordIterator* it = nullptr;
	if (impObj()->newIterator(it) != Acad::eOk || !it) return out;
	for (it->start(true, true); !it->done(); it->step(true, true)) {
		AcDbObjectId eid;
		if (it->getEntityId(eid) != Acad::eOk || eid.isNull()) continue;
		AcDbEntity* pEnt = nullptr;
		if (acdbOpenObject(pEnt, eid, AcDb::kForRead, false) != Acad::eOk || !pEnt) continue;
		AcDbAttributeDefinition* pAd = AcDbAttributeDefinition::cast(pEnt);
		if (!pAd) { pEnt->close(); continue; }
		AcGePoint3d pos = pAd->position();
		py::dict d;
		d["tag"] = pAd->tag();
		d["txt"] = pAd->textString();
		d["x"] = pos.x; d["y"] = pos.y; d["z"] = pos.z;
		d["h"] = pAd->height();
		d["rot"] = pAd->rotation();
		d["ha"] = (int)pAd->horizontalMode();
		d["va"] = (int)pAd->verticalMode();
		d["inv"] = pAd->isInvisible();
		d["ly"] = pAd->layer();
		d["ci"] = (int)pAd->colorIndex();
		out.append(d);
		pEnt->close();
	}
	delete it;
	return out;
}

std::vector<uint8_t> PyDbBlockTableRecord::getPreviewIcon() const {
	AcArray<Adesk::UInt8> bytes;
	if (!PyWarnIfError(impObj()->getPreviewIcon(bytes), L"AcDbBlockTableRecord::getPreviewIcon"))
		return {};
	return std::vector<uint8_t>(bytes.asArrayPtr(), bytes.asArrayPtr() + bytes.length());
}

bool  PyDbBlockTableRecord::setPreviewIcon(const std::vector<uint8_t>& bytes) {
	if (!impObj()) return false;
	AcArray<Adesk::UInt8> icon;
	icon.setLogicalLength(static_cast<int>(bytes.size()));
	if (!bytes.empty())
		memcpy(icon.asArrayPtr(), bytes.data(), bytes.size());
	return PyWarnIfError(impObj()->setPreviewIcon(icon), L"AcDbBlockTableRecord::setPreviewIcon");
}

void bind_PyDbBlockTableRecord(pym& m) {
	py::class_<PyDbBlockTableRecord, PyDbSymbolTableRecord>(m, "PyDbBlockTableRecord")
		.def(py::init<>([]{ return new PyDbBlockTableRecord(new AcDbBlockTableRecord()); }), "创建新的空块表记录（用于建自定义块）")
		.def(py::init<AcDbBlockTableRecord*>(), py::arg("p"))
		// 从 ObjectId(oldId) 打开块表记录（对应 C++ acdbOpenObject，无事务；命令上下文命令级事务自动提交）
		// 抗重复打开：同一对象已被 Python 侧以读打开时，再请求写会拿到 eWasOpenForRead，
		// 这里走打开缓存做 upgradeOpen，返回同一个底层指针（与 PyRx 的打开缓存语义一致）。
		.def(py::init([](uint64_t oldId, int mode) {
			AcDbObjectId objId(reinterpret_cast<AcDbStub*>(oldId));
			Acad::ErrorStatus es = Acad::eOk;
			AcDbObject* pObj = PyDbOpenCache::Open(objId, static_cast<AcDb::OpenMode>(mode), es);
			if ((es != Acad::eOk || !pObj) && (es == Acad::eWasOpenForRead || es == Acad::eWasOpenForWrite)) {
				es = Acad::eOk;
				pObj = PyDbOpenCache::Get(objId, static_cast<AcDb::OpenMode>(mode), es);
			}
			return new PyDbBlockTableRecord((es == Acad::eOk && pObj)
				? static_cast<AcDbBlockTableRecord*>(pObj) : nullptr);
		}), py::arg("old_id"), py::arg("mode") = 1, "从 ObjectId(oldId) 裸打开块表记录（无事务，命令上下文自动提交；失败返回空对象")
		PyWrapper_BIND(DbBlockTableRecord)
		.def("comments", &PyDbBlockTableRecord::comments)
		.def("setComments", &PyDbBlockTableRecord::setComments, py::arg("comments"))
		.def("pathName", &PyDbBlockTableRecord::pathName)
		.def("setPathName", &PyDbBlockTableRecord::setPathName, py::arg("path"))
		.def("origin", &PyDbBlockTableRecord::origin)
		.def("setOrigin", &PyDbBlockTableRecord::setOrigin, py::arg("pt"))
		.def("hasAttributeDefinitions", &PyDbBlockTableRecord::hasAttributeDefinitions)
		.def("hasPreviewIcon", &PyDbBlockTableRecord::hasPreviewIcon)
		.def("isAnonymous", &PyDbBlockTableRecord::isAnonymous)
		.def("isFromExternalReference", &PyDbBlockTableRecord::isFromExternalReference)
		.def("isFromOverlayReference", &PyDbBlockTableRecord::isFromOverlayReference)
		.def("setIsFromOverlayReference", &PyDbBlockTableRecord::setIsFromOverlayReference, py::arg("bIsOverlay"))
		.def("isLayout", &PyDbBlockTableRecord::isLayout)
		.def("getLayoutId", &PyDbBlockTableRecord::getLayoutId)
		.def("setLayoutId", &PyDbBlockTableRecord::setLayoutId, py::arg("layoutId"))
		.def("isUnloaded", &PyDbBlockTableRecord::isUnloaded)
		.def("setIsUnloaded", &PyDbBlockTableRecord::setIsUnloaded, py::arg("isUnloaded"))
		.def("xrefStatus", &PyDbBlockTableRecord::xrefStatus)
		.def("getBlockReferenceIds", &PyDbBlockTableRecord::getBlockReferenceIds, py::arg("directOnly") = true, py::arg("forceValidity") = false)
		.def("getErasedBlockReferenceIds", &PyDbBlockTableRecord::getErasedBlockReferenceIds)
		.def("assumeOwnershipOf", &PyDbBlockTableRecord::assumeOwnershipOf, py::arg("entitiesToMove"))
		.def("blockScaling", &PyDbBlockTableRecord::blockScaling)
		.def("setBlockScaling", &PyDbBlockTableRecord::setBlockScaling, py::arg("blockScaling"))
		.def("explodable", &PyDbBlockTableRecord::explodable)
		.def("setExplodable", &PyDbBlockTableRecord::setExplodable, py::arg("bExplodable"))
		.def("blockInsertUnits", &PyDbBlockTableRecord::blockInsertUnits)
		.def("setBlockInsertUnits", &PyDbBlockTableRecord::setBlockInsertUnits, py::arg("insunits"))
		.def("postProcessAnnotativeBTR", &PyDbBlockTableRecord::postProcessAnnotativeBTR, py::arg("bqueryOnly") = false, py::arg("bScale") = true)
		.def("addAnnoScalestoBlkRefs", &PyDbBlockTableRecord::addAnnoScalestoBlkRefs, py::arg("bScale") = false)
		.def("appendEntity", &PyDbBlockTableRecord::appendEntity, py::arg("entity"), "向该块记录（模型/图纸空间）追加一个实体")
		.def("appendAcDbEntity", [](PyDbBlockTableRecord& self, PyDbEntity& entity) {
			// 不能直接用 ARX 原名别名：两参重载入库后实体仍保持打开态，后续遍历/改属性会
			// eWasOpenForWrite 打不开。这里与 appendEntity 一致——入库即关闭并释放包装。
			AcDbObjectId outId;
			Acad::ErrorStatus es = self.impObj()->appendAcDbEntity(outId, entity.impObj());
			if (es != Acad::eOk) return PyWarnIfError(es, L"AcDbBlockTableRecord::appendAcDbEntity");
			entity.impObj()->close();
			entity.detachDbObject();
			return true;
		}, py::arg("entity"), "向该块记录追加一个实体（ARX 原名别名，返回 bool；入库后自动关闭）")
		.def("appendAcDbEntityId", &PyDbBlockTableRecord::appendAcDbEntityId, py::arg("entity"), "追加实体并返回新实体 ObjectId（oldId），失败返回 0")
		.def("getPreviewIcon", &PyDbBlockTableRecord::getPreviewIcon, "返回预览图标原始字节（BMP），失败返回空 list")
		.def("setPreviewIcon", &PyDbBlockTableRecord::setPreviewIcon, py::arg("bytes"), "设置预览图标原始字节（BMP）")
		// 遍历块内所有实体 id（BlockTableRecordIterator 封装）
		.def("entityIds", [](PyDbBlockTableRecord& self) -> py::list {
			py::list out;
			AcDbBlockTableRecordIterator* it = nullptr;
			if (self.impObj()->newIterator(it) != Acad::eOk || !it) return out;
			for (it->start(true, true); !it->done(); it->step(true, true)) {
				AcDbObjectId eid;
				if (it->getEntityId(eid) == Acad::eOk && !eid.isNull())
					out.append(static_cast<uint64_t>(eid.asOldId()));
			}
			delete it;
			return out;
		}, "遍历块记录内所有实体的 ObjectId 列表")
		// ---- 索引访问（Iterator 折叠进 BTR 本体，不再单独暴露迭代器类）----
		.def("entityAt", [](PyDbBlockTableRecord& self, int index, int mode) -> PyDbEntity {
			if (!self.impObj()) return PyDbEntity(nullptr);
			AcDbBlockTableRecordIterator* it = nullptr;
			if (self.impObj()->newIterator(it, true, true) != Acad::eOk || !it) return PyDbEntity(nullptr);
			int total = 0;
			for (it->start(true, true); !it->done(); it->step(true, true)) ++total;
			if (index < 0) index += total;
			if (index < 0 || index >= total) { delete it; throw py::index_error("BlockTableRecord 索引越界"); }
			int i = 0;
			for (it->start(true, true); !it->done(); it->step(true, true), ++i) {
				if (i != index) continue;
				AcDbEntity* pEnt = nullptr;
				Acad::ErrorStatus es = it->getEntity(pEnt, static_cast<AcDb::OpenMode>(mode), false);
				if (es != Acad::eOk || !pEnt) {
					pEnt = nullptr;
					es = it->getEntity(pEnt, AcDb::kForRead, false);
					delete it;
					if (es != Acad::eOk || !pEnt) return PyDbEntity(nullptr);
					PyDbOpenCache::Track(pEnt, AcDb::kForRead);
					return PyDbEntity(pEnt);
				}
				delete it;
				PyDbOpenCache::Track(pEnt, static_cast<AcDb::OpenMode>(mode));
				return PyDbEntity(pEnt);
			}
			delete it;
			return PyDbEntity(nullptr);
		}, py::arg("index"), py::arg("mode") = 1, "btr.entityAt(i)：按位置取实体（支持负索引，越界抛 IndexError）")
		.def("refIds", [](PyDbBlockTableRecord& self) -> py::list {
			py::list out;
			if (!self.impObj()) return out;
			AcDbBlockReferenceIdIterator* it = nullptr;
			if (self.impObj()->newBlockReferenceIdIterator(it) != Acad::eOk || !it) return out;
			for (it->start(); !it->done(); it->step()) {
				AcDbObjectId id;
				if (it->getBlockReferenceId(id) == Acad::eOk && !id.isNull())
					out.append(static_cast<uint64_t>(id.asOldId()));
			}
			delete it;
			return out;
		}, "块内所有块参照的 ObjectId 列表（原 BlockReferenceIdIterator 折叠进本体）")		.def("numEntities", &PyDbBlockTableRecord::numEntities, "块内实体数量（不含已删除）")
		.def("__len__", &PyDbBlockTableRecord::numEntities, "len(btr)：块内实体数量")
		.def("attdefData", &PyDbBlockTableRecord::attdefData,
			"块内 ATTDEF 列表：[{tag, txt, x, y, z, h, rot, ha, va, inv, ly, ci}]")
		.def("getEntityById", &PyDbBlockTableRecord::openEntityById,
			py::arg("id"), py::arg("mode") = 1, py::return_value_policy::take_ownership,
			"按 ObjectId(oldId) 打开块内实体；mode 0=只读 1=写（默认写），失败返回空包装")
		.def("__getitem__", [](PyDbBlockTableRecord& self, uint64_t id) {
			return self.getEntityById(id, 1);
		}, py::arg("id"), "btr[id]：按 id 打开实体（写模式）")
		.def("__contains__", [](PyDbBlockTableRecord& self, uint64_t id) {
			return self.getEntityById(id, 0).impObj() != nullptr;
		}, py::arg("id"), "id in btr：该 id 是否为块内可打开实体")
		.def("__iter__", [](PyDbBlockTableRecord& self) {
			return py::cast(new PyDbBlockTableRecordEntityIter(self, 1, true),
				py::return_value_policy::take_ownership);
		}, "for ent in btr：直接遍历实体包装")
		.def("iterEntities", [](PyDbBlockTableRecord& self, int mode, bool skipDeleted) {
			return py::cast(new PyDbBlockTableRecordEntityIter(self, mode, skipDeleted),
				py::return_value_policy::take_ownership);
		}, py::arg("mode") = 1, py::arg("skipDeleted") = true,
			"显式取实体遍历器；mode 0=只读 1=写")
		;
}
