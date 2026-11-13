#include "stdafx.h"
#include "dbapserv.h"   // acdbOpenObject（裸打开）+ AcDbTransactionManager
#include "PyErrorStatusException.h"
#include "PyUtils.h"
#include "PyDbBlockTableRecord.h"

PyWrapper_Cpp(DbBlockTableRecord);

AcString PyDbBlockTableRecord::comments() const { AcString s; if(!PyWarnIfError(impObj()->comments(s), L"AcDbBlockTableRecord::comments")) return AcString(); return s; }
bool  PyDbBlockTableRecord::setComments(const AcString& comments) { return PyWarnIfError(impObj()->setComments(comments.kwszPtr()), L"AcDbBlockTableRecord::setComments"); }
AcString PyDbBlockTableRecord::pathName() const { AcString s; if(!PyWarnIfError(impObj()->pathName(s), L"AcDbBlockTableRecord::pathName")) return AcString(); return s; }
bool  PyDbBlockTableRecord::setPathName(const AcString& path) { return PyWarnIfError(impObj()->setPathName(path.kwszPtr()), L"AcDbBlockTableRecord::setPathName"); }
AcGePoint3d PyDbBlockTableRecord::origin() const { return impObj()->origin(); }
bool  PyDbBlockTableRecord::setOrigin(const AcGePoint3d& pt) { return PyWarnIfError(impObj()->setOrigin(pt), L"AcDbBlockTableRecord::setOrigin"); }
bool PyDbBlockTableRecord::hasAttributeDefinitions() const { return impObj()->hasAttributeDefinitions(); }
bool PyDbBlockTableRecord::hasPreviewIcon() const { return impObj()->hasPreviewIcon(); }
bool PyDbBlockTableRecord::isAnonymous() const { return impObj()->isAnonymous(); }
bool PyDbBlockTableRecord::isFromExternalReference() const { return impObj()->isFromExternalReference(); }
bool PyDbBlockTableRecord::isFromOverlayReference() const { return impObj()->isFromOverlayReference(); }
bool  PyDbBlockTableRecord::setIsFromOverlayReference(bool bIsOverlay) { return PyWarnIfError(impObj()->setIsFromOverlayReference(bIsOverlay), L"AcDbBlockTableRecord::setIsFromOverlayReference"); }
bool PyDbBlockTableRecord::isLayout() const { return impObj()->isLayout(); }
uint64_t PyDbBlockTableRecord::getLayoutId() const { return static_cast<uint64_t>(impObj()->getLayoutId().asOldId()); }
bool  PyDbBlockTableRecord::setLayoutId(uint64_t layoutId) {
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(layoutId));
	return PyWarnIfError(impObj()->setLayoutId(id), L"AcDbBlockTableRecord::setLayoutId");
}
bool PyDbBlockTableRecord::isUnloaded() const { return impObj()->isUnloaded(); }
bool  PyDbBlockTableRecord::setIsUnloaded(bool isUnloaded) { return PyWarnIfError(impObj()->setIsUnloaded(isUnloaded), L"AcDbBlockTableRecord::setIsUnloaded"); }
int PyDbBlockTableRecord::xrefStatus() const { return static_cast<int>(impObj()->xrefStatus()); }

py::list PyDbBlockTableRecord::getBlockReferenceIds(bool directOnly, bool forceValidity) const {
	AcDbObjectIdArray ids;
	if(!PyWarnIfError(impObj()->getBlockReferenceIds(ids, directOnly, forceValidity), L"AcDbBlockTableRecord::getBlockReferenceIds")) return py::list();
	return py::cast(ids);
}

py::list PyDbBlockTableRecord::getErasedBlockReferenceIds() const {
	AcDbObjectIdArray ids;
	if(!PyWarnIfError(impObj()->getErasedBlockReferenceIds(ids), L"AcDbBlockTableRecord::getErasedBlockReferenceIds")) return py::list();
	return py::cast(ids);
}

bool  PyDbBlockTableRecord::assumeOwnershipOf(const AcDbObjectIdArray& entitiesToMove) {
	return PyWarnIfError(impObj()->assumeOwnershipOf(entitiesToMove), L"AcDbBlockTableRecord::assumeOwnershipOf");
}

int PyDbBlockTableRecord::blockScaling() const { return static_cast<int>(impObj()->blockScaling()); }
bool  PyDbBlockTableRecord::setBlockScaling(int blockScaling) {
	return PyWarnIfError(impObj()->setBlockScaling(static_cast<AcDbBlockTableRecord::BlockScaling>(blockScaling)), L"AcDbBlockTableRecord::setBlockScaling");
}
bool PyDbBlockTableRecord::explodable() const { return impObj()->explodable(); }
bool  PyDbBlockTableRecord::setExplodable(bool bExplodable) { return PyWarnIfError(impObj()->setExplodable(bExplodable), L"AcDbBlockTableRecord::setExplodable"); }
int PyDbBlockTableRecord::blockInsertUnits() const { return static_cast<int>(impObj()->blockInsertUnits()); }
bool  PyDbBlockTableRecord::setBlockInsertUnits(int insunits) { return PyWarnIfError(impObj()->setBlockInsertUnits(static_cast<AcDb::UnitsValue>(insunits)), L"AcDbBlockTableRecord::setBlockInsertUnits"); }
int PyDbBlockTableRecord::postProcessAnnotativeBTR(bool bqueryOnly, bool bScale) {
	int stripCnt = 0;
	if(!PyWarnIfError(impObj()->postProcessAnnotativeBTR(stripCnt, bqueryOnly, bScale), L"AcDbBlockTableRecord::postProcessAnnotativeBTR")) return 0;
	return stripCnt;
}
bool  PyDbBlockTableRecord::addAnnoScalestoBlkRefs(bool bScale) { return PyWarnIfError(impObj()->addAnnoScalestoBlkRefs(bScale), L"AcDbBlockTableRecord::addAnnoScalestoBlkRefs"); }

bool  PyDbBlockTableRecord::appendEntity(PyDbEntity& pEntity) {
	// 一个调用完成 C++ 三步：appendAcDbEntity + addNewlyCreatedDBRObject + close。
	// Python 顶层/非 ARX 命令回调无命令级事务，故无显式事务时自动启动一个事务并在提交后释放。
	AcDbObjectId outId;
	AcDbDatabase* db = impObj()->database();
	AcDbTransactionManager* pTm = db ? db->transactionManager() : nullptr;
	AcTransaction* pTrans = pTm ? pTm->startTransaction() : nullptr;
	Acad::ErrorStatus es = impObj()->appendAcDbEntity(outId, pEntity.impObj());
	if (es == Acad::eOk && pTm) pTm->addNewlyCreatedDBRObject(pEntity.impObj());
	pEntity.detachDbObject();
	if (pTrans) pTm->endTransaction();
	if (es != Acad::eOk) { PyWarnIfError(es, L"AcDbBlockTableRecord::appendEntity"); return false; }
	return true;
}

uint64_t PyDbBlockTableRecord::appendAcDbEntityId(PyDbEntity& pEntity) {
	AcDbObjectId outId;
	AcDbDatabase* db = impObj()->database();
	AcDbTransactionManager* pTm = db ? db->transactionManager() : nullptr;
	AcTransaction* pTrans = pTm ? pTm->startTransaction() : nullptr;
	Acad::ErrorStatus es = impObj()->appendAcDbEntity(outId, pEntity.impObj());
	if (es == Acad::eOk && pTm) pTm->addNewlyCreatedDBRObject(pEntity.impObj());
	pEntity.detachDbObject();
	if (pTrans) pTm->endTransaction();
	if (es != Acad::eOk) { PyWarnIfError(es, L"AcDbBlockTableRecord::appendAcDbEntityId"); return 0ULL; }
	return outId.isNull() ? 0ULL : static_cast<uint64_t>(outId.asOldId());
}

std::vector<uint8_t> PyDbBlockTableRecord::getPreviewIcon() const {
	AcArray<Adesk::UInt8> bytes;
	if (!PyWarnIfError(impObj()->getPreviewIcon(bytes), L"AcDbBlockTableRecord::getPreviewIcon"))
		return {};
	return std::vector<uint8_t>(bytes.asArrayPtr(), bytes.asArrayPtr() + bytes.length());
}

bool  PyDbBlockTableRecord::setPreviewIcon(const std::vector<uint8_t>& bytes) {
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
		// 从 ObjectId(oldId) 裸打开块表记录（对应 C++ acdbOpenObject，无事务；命令上下文命令级事务自动提交）
		.def(py::init([](uint64_t oldId, int mode) {
			AcDbObject* pObj = nullptr;
			AcDbObjectId objId(reinterpret_cast<AcDbStub*>(oldId));
			Acad::ErrorStatus es = acdbOpenObject(pObj, objId, static_cast<AcDb::OpenMode>(mode));
			return new PyDbBlockTableRecord((es == Acad::eOk) ? static_cast<AcDbBlockTableRecord*>(pObj) : nullptr);
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
			return PyWarnIfError(self.impObj()->appendAcDbEntity(entity.impObj()), L"AcDbBlockTableRecord::appendAcDbEntity");
		}, py::arg("entity"), "向该块记录追加一个实体（ARX 原名别名，返回 bool）")
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
		;
}
