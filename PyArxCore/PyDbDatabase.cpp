#include "stdafx.h"
#include "logPath.h"
#include <fstream>
#include <acdocman.h>
#include "PyDbDatabase.h"
#include "PyErrorStatusException.h"
#include "PyDbObject.h"
#include "PyDbDate.h"
#include "PyDbTransactionManager.h"
#include "PyDbDbLayerStateManager.h"
#include "PyDbObjectContextManager.h"
#include "PyDbBlockTable.h"
#include "PyDbLayerTable.h"
#include "PyDbTextStyleTable.h"
#include "PyDbLinetypeTable.h"
#include "PyDbViewTable.h"
#include "PyDbUCSTable.h"
#include "PyDbViewportTable.h"
#include "PyDbRegAppTable.h"
#include "PyDbDimStyleTable.h"
#include "PyDbDictionary.h"
#include "PyDbTypeCaster.h"

PyWrapper_Cpp(DbDatabase)

bool PyDbDatabase::isBeingDestroyed() const
{
	return impObj()->isBeingDestroyed();
}

AcDbObjectId  PyDbDatabase::addAcDbObject(PyDbObject& pObj)
{
	AcDbObjectId result;
	CheckErrorStatus(impObj()->addAcDbObject(result, pObj.impObj()));
	return result;
}

// --- 便捷方法：添加入库 ---
uint64_t PyDbDatabase::addToBlock(uint64_t blockId, PyDbObject& entity) {
	// 防呆：id 为 0、对象不是实体时立刻抛可读错误，避免拿空 id 去 append 触发 ARX 致命断言。
	if (!blockId)
		throw std::runtime_error("addToBlock: blockId 无效(0)，请先用 db.currentSpaceId() 或 bt.getIdAt(name) 取有效 id");
	AcDbEntity* pEnt = dynamic_cast<AcDbEntity*>(entity.impObj());
	if (!pEnt)
		throw std::runtime_error("addToBlock: 传入对象不是 AcDbEntity（块表记录/符号表记录不能作为实体入库）");

	AcDbObjectId btrId;
	btrId.setFromOldId(static_cast<Adesk::IntDbId>(blockId));
	AcDbBlockTableRecordPointer btr(btrId, AcDb::kForWrite);
	if (btr.openStatus() != Acad::eOk) {
		AcString errText = acadErrorStatusText(btr.openStatus());
		const char* errUtf8 = errText.utf8Ptr();
		throw std::runtime_error(std::string("addToBlock: 块表记录打不开(kForWrite): ") +
			(errUtf8 ? errUtf8 : "") + "；可能该记录已被别处写打开，或属于其它图形");
	}

	// 经典 ARX 写法：appendAcDbEntity + 关闭实体，不碰事务管理器。
	// 旧实现走 startTransaction/addNewlyCreatedDBRObject/endTransaction，函数体全部成功但返回后
	// pybind 报 unknown exception 且实体未入库（2026-11-17 实测）。
	AcDbObjectId newId;
	Acad::ErrorStatus es = btr->appendAcDbEntity(newId, pEnt);
	if (es != Acad::eOk) {
		AcString errText = acadErrorStatusText(es);
		const char* errUtf8 = errText.utf8Ptr();
		throw std::runtime_error(std::string("addToBlock: appendAcDbEntity 失败: ") +
			(errUtf8 ? errUtf8 : "") + "（实体可能已入库，或块记录正处于其它打开状态）");
	}
	pEnt->close();             // 入库后必须关闭，否则对象保持打开态，后续属性写入报 eWasOpenForWrite
	entity.detachDbObject();   // 包装释放所有权，避免 Python 侧析构 delete 已入库实体
	return static_cast<uint64_t>(newId.asOldId());
}

uint64_t PyDbDatabase::addToModelspace(PyDbObject& entity) {
	// 直接用 acdbSymUtil 更简单
	AcDbObjectId modelSpaceId = acdbSymUtil()->blockModelSpaceId(impObj());
	return addToBlock(static_cast<uint64_t>(modelSpaceId.asOldId()), entity);
}

uint64_t PyDbDatabase::addToCurrentspace(PyDbObject& entity) {
	if (!impObj()) return 0ULL;
	return addToBlock(static_cast<uint64_t>(impObj()->currentSpaceId().asOldId()), entity);
}

// --- 遍历对象 ---
py::list PyDbDatabase::objectIds(const std::string& className) const {
	py::list result;
	AcDbBlockTableRecordPointer modelSpace(acdbSymUtil()->blockModelSpaceId(impObj()), AcDb::kForRead);
	if (modelSpace.openStatus() != Acad::eOk) return result;

	AcDbBlockTableRecordIterator* pIter = nullptr;
	if (modelSpace->newIterator(pIter) != Acad::eOk) return result;

	// 空串或 "AcDbEntity" 视为通配，返回模型空间全部对象；否则按注册类名（含派生类）匹配。
	const bool wildcard = className.empty() || className == "AcDbEntity";
	AcRxClass* pTargetCls = nullptr;
	if (!wildcard) {
		const AcString target(className.c_str(), AcString::Utf8);
		pTargetCls = AcRxClass::cast(acrxClassDictionary->at(target.kwszPtr()));
		if (pTargetCls == nullptr) { delete pIter; return result; }  // 未知类名：返回空列表
	}

	for (; !pIter->done(); pIter->step()) {
		AcDbObjectId id;
		pIter->getEntityId(id);
		if (wildcard) {
			result.append(static_cast<uint64_t>(id.asOldId()));
			continue;
		}
		AcDbEntityPointer pEnt(id, AcDb::kForRead);
		if (pEnt.openStatus() != Acad::eOk) continue;
		if (pEnt->isKindOf(pTargetCls))
			result.append(static_cast<uint64_t>(id.asOldId()));
	}
	delete pIter;
	return result;
}

// --- 系统变量统一访问 ---
py::object PyDbDatabase::getSysVar(const std::string& name) const {
	struct resbuf rb;
	memset(&rb, 0, sizeof(rb));
	if (acedGetVar(AcString(name.c_str()), &rb) != RTNORM) return py::none();
	py::object result;
	switch (rb.restype) {
		case RTSHORT: result = py::int_(rb.resval.rint); break;
		case RTLONG: result = py::int_(rb.resval.rlong); break;
		case RTINT64: result = py::int_(rb.resval.mnInt64); break;
		case RTREAL:
		case RTANG: result = py::float_(rb.resval.rreal); break;
		case RTSTR: result = py::cast(AcString(rb.resval.rstring)); break;
		case RTPOINT: result = py::make_tuple(rb.resval.rpoint[0], rb.resval.rpoint[1]); break;
		case RT3DPOINT: result = py::make_tuple(rb.resval.rpoint[0], rb.resval.rpoint[1], rb.resval.rpoint[2]); break;
		case RTENAME:
		case RTPICKS:
			// 命名对象/实体名/选择集名二元组（两个 oldId），值类型，无字符串分配
			result = py::make_tuple(py::int_(rb.resval.rlname[0]), py::int_(rb.resval.rlname[1]));
			break;
		default: result = py::none(); break;
	}
	// RTSTR 由 acedGetVar 动态分配，统一释放，避免其他分支遗漏
	if (rb.restype == RTSTR && rb.resval.rstring) acutRelRb(&rb);
	return result;
}

bool PyDbDatabase::setSysVar(const std::string& name, const py::object& value) {
	struct resbuf rb;
	if (py::isinstance<py::int_>(value)) {
		rb.restype = RTSHORT;
		rb.resval.rint = value.cast<int>();
	} else if (py::isinstance<py::float_>(value)) {
		rb.restype = RTREAL;
		rb.resval.rreal = value.cast<double>();
	} else if (py::isinstance<py::str>(value)) {
		AcString s = value.cast<AcString>();
		rb.restype = RTSTR;
		rb.resval.rstring = const_cast<ACHAR*>(s.constPtr());
	} else {
		return false;
	}
	return acedSetVar(AcString(name.c_str()), &rb) == RTNORM;
}

// --- 文件读写 ---
bool PyDbDatabase::dxfIn(const std::string& filename) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->dxfIn(AcString(filename.c_str())), L"AcDbDatabase::dxfIn");
}

bool PyDbDatabase::dxfOut(const std::string& filename, int precision) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->dxfOut(AcString(filename.c_str()), precision), L"AcDbDatabase::dxfOut");
}

Acad::ErrorStatus PyDbDatabase::markObjectNonPersistent(AcDbObjectId id, bool value)
{
	return AcDbDatabase::markObjectNonPersistent(id, value);
}

bool PyDbDatabase::isObjectNonPersistent(AcDbObjectId id)
{
	return AcDbDatabase::isObjectNonPersistent(id);
}

AcDbBlockTable* PyDbDatabase::getBlockTable(AcDb::OpenMode mode)
{
	return getSymbolTable<AcDbBlockTable>(mode);
}

AcDbLayerTable* PyDbDatabase::getLayerTable(AcDb::OpenMode mode)
{
	return getSymbolTable<AcDbLayerTable>(mode);
}

AcDbTextStyleTable* PyDbDatabase::getTextStyleTable(AcDb::OpenMode mode)
{
	return getSymbolTable<AcDbTextStyleTable>(mode);
}

AcDbLinetypeTable* PyDbDatabase::getLinetypeTable(AcDb::OpenMode mode)
{
	return getSymbolTable<AcDbLinetypeTable>(mode);
}

AcDbViewTable* PyDbDatabase::getViewTable(AcDb::OpenMode mode)
{
	return getSymbolTable<AcDbViewTable>(mode);
}

AcDbUCSTable* PyDbDatabase::getUCSTable(AcDb::OpenMode mode)
{
	return getSymbolTable<AcDbUCSTable>(mode);
}

AcDbViewportTable* PyDbDatabase::getViewportTable(AcDb::OpenMode mode)
{
	return getSymbolTable<AcDbViewportTable>(mode);
}

AcDbRegAppTable* PyDbDatabase::getRegAppTable(AcDb::OpenMode mode)
{
	return getSymbolTable<AcDbRegAppTable>(mode);
}

AcDbDimStyleTable* PyDbDatabase::getDimStyleTable(AcDb::OpenMode mode)
{
	return getSymbolTable<AcDbDimStyleTable>(mode);
}


// dictionary
AcDbDictionary* PyDbDatabase::getGroupDictionary(AcDb::OpenMode mode)
{
	AcDbDictionary* pDict = nullptr;
	CheckErrorStatus(impObj()->getGroupDictionary(pDict, mode));
	return pDict;
}

AcDbDictionary* PyDbDatabase::getMLStyleDictionary(AcDb::OpenMode mode)
{
	AcDbDictionary* pDict = nullptr;
	CheckErrorStatus(impObj()->getMLStyleDictionary(pDict, mode));
	return pDict;
}


AcDbDictionary* PyDbDatabase::getLayoutDictionary(AcDb::OpenMode mode)
{
	AcDbDictionary* pDict = nullptr;
	CheckErrorStatus(impObj()->getLayoutDictionary(pDict, mode));
	return pDict;
}


AcDbDictionaryWithDefault* PyDbDatabase::getPlotStyleNameDictionary(AcDb::OpenMode mode)
{
	AcDbDictionaryWithDefault* pDict = nullptr;
	CheckErrorStatus(impObj()->getPlotStyleNameDictionary(pDict, mode));
	return pDict;
}

AcDbDictionary* PyDbDatabase::getMaterialDictionary(AcDb::OpenMode mode)
{
	AcDbDictionary* pDict = nullptr;
	CheckErrorStatus(impObj()->getMaterialDictionary(pDict, mode));
	return pDict;
}


AcDbDictionary* PyDbDatabase::getVisualStyleDictionary(AcDb::OpenMode mode)
{
	AcDbDictionary* pDict = nullptr;
	CheckErrorStatus(impObj()->getVisualStyleDictionary(pDict, mode));
	return pDict;
}

AcDbDictionary* PyDbDatabase::getColorDictionary(AcDb::OpenMode mode)
{
	AcDbDictionary* pDict = nullptr;
	CheckErrorStatus(impObj()->getColorDictionary(pDict, mode));
	return pDict;
}

AcDbDictionary* PyDbDatabase::getScaleListDictionary(AcDb::OpenMode mode)
{
	AcDbDictionary* pDict = nullptr;
	CheckErrorStatus(impObj()->getScaleListDictionary(pDict, mode));
	return pDict;
}

AcDbDictionary* PyDbDatabase::getNamedObjectsDictionary(AcDb::OpenMode mode)
{
	AcDbDictionary* pDict = nullptr;
	CheckErrorStatus(impObj()->getNamedObjectsDictionary(pDict, mode));
	return pDict;
}

AcDbDictionary* PyDbDatabase::getPlotSettingsDictionary(AcDb::OpenMode mode)
{
	AcDbDictionary* pDict = nullptr;
	CheckErrorStatus(impObj()->getPlotSettingsDictionary(pDict, mode));
	return pDict;
}

AcDbDictionary* PyDbDatabase::getDataLinkDictionary(AcDb::OpenMode mode)
{
	AcDbDictionary* pDict = nullptr;
	CheckErrorStatus(impObj()->getDataLinkDictionary(pDict, mode));
	return pDict;
}


AcDbObjectId PyDbDatabase::getSectionManager(void) const
{
	return impObj()->getSectionManager();
}

AcDbSectionManager* PyDbDatabase::getSectionManager(AcDb::OpenMode mode) const
{
	AcDbSectionManager* pMgr = nullptr;
	CheckErrorStatus(impObj()->getSectionManager(pMgr, mode));
	return pMgr;
}

AcDbDataLinkManager* PyDbDatabase::getDataLinkManager(void) const
{
	return impObj()->getDataLinkManager();
}

Acad::ErrorStatus PyDbDatabase::updateDataLink(AcDb::UpdateDirection nDir, AcDb::UpdateOption nOption)
{
	return impObj()->updateDataLink(nDir, nOption);
}


AcDbObjectId PyDbDatabase::dataLinkDictionaryId(void) const
{
	return impObj()->dataLinkDictionaryId();
}
AcDbObjectId PyDbDatabase::blockTableId() const
{
	return impObj()->blockTableId();
}

AcDbObjectId PyDbDatabase::layerTableId() const
{
	return impObj()->layerTableId();
}

AcDbObjectId PyDbDatabase::textStyleTableId() const
{
	return impObj()->textStyleTableId();
}

AcDbObjectId PyDbDatabase::linetypeTableId() const
{
	return impObj()->linetypeTableId();
}

AcDbObjectId PyDbDatabase::viewTableId() const
{
	return impObj()->viewTableId();
}

AcDbObjectId PyDbDatabase::UCSTableId() const
{
	return impObj()->UCSTableId();
}

AcDbObjectId PyDbDatabase::viewportTableId() const
{
	return impObj()->viewportTableId();
}

AcDbObjectId PyDbDatabase::regAppTableId() const
{
	return impObj()->regAppTableId();
}

AcDbObjectId PyDbDatabase::dimStyleTableId() const
{
	return impObj()->dimStyleTableId();
}

AcDbObjectId PyDbDatabase::mLStyleDictionaryId() const
{
	return impObj()->mLStyleDictionaryId();
}

AcDbObjectId PyDbDatabase::groupDictionaryId() const
{
	return impObj()->groupDictionaryId();
}

AcDbObjectId PyDbDatabase::layoutDictionaryId() const
{
	return impObj()->layoutDictionaryId();
}

AcDbObjectId PyDbDatabase::plotStyleNameDictionaryId() const
{
	return impObj()->plotStyleNameDictionaryId();
}

AcDbObjectId PyDbDatabase::materialDictionaryId() const
{
	return impObj()->materialDictionaryId();
}

AcDbObjectId PyDbDatabase::visualStyleDictionaryId() const
{
	return impObj()->visualStyleDictionaryId();
}

AcDbObjectId PyDbDatabase::colorDictionaryId() const
{
	return impObj()->colorDictionaryId();
}

AcDbObjectId PyDbDatabase::scaleListDictionaryId() const
{
	return impObj()->scaleListDictionaryId();
}

AcDbObjectId PyDbDatabase::namedObjectsDictionaryId() const
{
	return impObj()->namedObjectsDictionaryId();
}

AcDbObjectId PyDbDatabase::plotSettingsDictionaryId() const
{
	return impObj()->plotSettingsDictionaryId();
}




AcDbObjectId PyDbDatabase::layerZero() const
{
	return impObj()->layerZero();
}

AcDbObjectId PyDbDatabase::continuousLinetype() const
{
	return impObj()->continuousLinetype();
}

AcDbObjectId PyDbDatabase::byLayerLinetype() const
{
	return impObj()->byLayerLinetype();
}

AcDbObjectId PyDbDatabase::byBlockLinetype() const
{
	return impObj()->byBlockLinetype();
}

AcDbObjectId PyDbDatabase::globalMaterial() const
{
	return impObj()->globalMaterial();
}

AcDbObjectId PyDbDatabase::byLayerMaterial() const
{
	return impObj()->byLayerMaterial();
}

AcDbObjectId PyDbDatabase::byBlockMaterial() const
{
	return impObj()->byBlockMaterial();
}

AcDbObjectId PyDbDatabase::paperSpaceVportId() const
{
	return impObj()->paperSpaceVportId();
}

AcDbObjectId PyDbDatabase::currentSpaceId() const
{
	return impObj()->currentSpaceId();
}

Acad::ErrorStatus PyDbDatabase::reclaimMemoryFromErasedObjects(const PyDbObjectIdArray& erasedObjects)
{
	return impObj()->reclaimMemoryFromErasedObjects(VecToAcArray<AcDbObjectId>(erasedObjects));
}

PyDbObjectIdArray PyDbDatabase::purge(const PyDbObjectIdArray& ids)
{
	auto arr = VecToAcArray<AcDbObjectId>(ids);
	auto es = impObj()->purge(arr);
	if (es != Acad::eOk) {
		return ids;
	}
	return AcArrayToVec<AcDbObjectId>(arr);
}


AcDbObjectId PyDbDatabase::getPyDbObjectId(bool createIfNotFound, const AcDbHandle& objHandle, Adesk::UInt32 xRefId)
{
	AcDbObjectId retId;
	CheckErrorStatus(impObj()->getAcDbObjectId(retId, createIfNotFound, objHandle, xRefId));
	return retId;
}

AcString PyDbDatabase::classDxfName(const PyRxClass& pClass)
{
	return AcString(impObj()->classDxfName(pClass.impObj()));
}

bool PyDbDatabase::hasClass(const PyRxClass& pClass) const
{
	return impObj()->hasClass(pClass.impObj());
}

Acad::ErrorStatus PyDbDatabase::readDwgFile(AcString fileName, AcDbDatabase::OpenMode shmode, bool bAllowCPConversion, const AcString& wszPassword)
{
	return impObj()->readDwgFile(fileName.kwszPtr(), shmode, bAllowCPConversion, wszPassword.kwszPtr());
}

Acad::ErrorStatus PyDbDatabase::readDwgFile(AcDwgFileHandle* pDwgFile, bool bAllowCPConversion, const AcString& wszPassword)
{
	return impObj()->readDwgFile(pDwgFile, bAllowCPConversion, wszPassword.kwszPtr());
}


bool PyDbDatabase::needsRecovery() const {
	if (!impObj()) return false;
	return impObj()->needsRecovery();
}

void PyDbDatabase::setFullSaveRequired() {
	if (!impObj()) return;
	impObj()->setFullSaveRequired();
}

Acad::ErrorStatus PyDbDatabase::saveAs(AcString fileName, const SecurityParams* pSecParams)
{
	return impObj()->saveAs(fileName.kwszPtr(), pSecParams);
}

Acad::ErrorStatus PyDbDatabase::saveAs(AcString fileName, const bool bBakAndRename, const AcDb::AcDbDwgVersion dwgVer, const SecurityParams* pSecParams)
{
	return impObj()->saveAs(fileName.kwszPtr(), bBakAndRename, dwgVer, pSecParams);
}


Acad::ErrorStatus PyDbDatabase::saveAs(AcDwgFileHandle* pOutputFiler, const SecurityParams* pSecParams, bool crashing) {
	if (!impObj()) return Acad::eNullObjectPointer;

	return impObj()->saveAs(pOutputFiler, pSecParams, crashing);
}
Acad::ErrorStatus PyDbDatabase::save() {
	if (!impObj()) return Acad::eNullObjectPointer;
	// 原地保存：直接 saveAs 到当前文件名（同步 API），不走 sendStringToExecute QSAVE。
	// AcDbDatabase::save() 仅对新建空库有效；readDwgFile 打开的库和文档挂接库都必须用 saveAs。
	AcString fn = getFilename();
	if (!fn.isEmpty()) {
		return impObj()->saveAs(fn.kwszPtr(), nullptr);
	}
	return impObj()->save();
}
Acad::ErrorStatus PyDbDatabase::dxfIn(AcString dxfFilename, AcString logFilename) {
	if (!impObj()) return Acad::eNullObjectPointer;
	return impObj()->dxfIn(dxfFilename.kwszPtr(), logFilename.kwszPtr());
}
Acad::ErrorStatus PyDbDatabase::dxfIn(IAcReadStream* pInputStream, AcString logFilename) {
	if (!impObj()) return Acad::eNullObjectPointer;
	return impObj()->dxfIn(pInputStream, logFilename.kwszPtr());
}

Acad::ErrorStatus PyDbDatabase::dxfOut(AcString fileName, int precision, AcDb::AcDbDwgVersion dwgVer, bool saveThumbnailImage) {
	if (!impObj()) return Acad::eNullObjectPointer;
	return impObj()->dxfOut(fileName.kwszPtr(), precision, dwgVer, saveThumbnailImage);
}


Acad::ErrorStatus PyDbDatabase::dxfOutEx(AcString fileName, int precision, AcDb::AcDbDwgVersion dwgVer, int nFlags,
	int nCodePage) {
	return impObj()->dxfOutEx(fileName.kwszPtr(), precision, dwgVer, nFlags, static_cast<code_page_id>(nCodePage));
}

Acad::ErrorStatus PyDbDatabase::closeInput(bool bCloseFile) {
	if (!impObj()) return Acad::eNullObjectPointer;
	return impObj()->closeInput(bCloseFile);
}

int PyDbDatabase::approxNumObjects() const {
	if (!impObj()) return 0;
	return impObj()->approxNumObjects();
}
Adesk::Int32 PyDbDatabase::numberOfSaves() const {
	if (!impObj()) return 0;
	return impObj()->numberOfSaves();
}
AcString PyDbDatabase::originalFileName() const {
	if (!impObj()) return AcString();
	return AcString(impObj()->originalFileName());
}
AcDb::AcDbDwgVersion PyDbDatabase::lastSavedAsVersion() const {
	if (!impObj()) return {};
	return impObj()->lastSavedAsVersion();
}
AcDb::AcDbDwgVersion PyDbDatabase::originalFileVersion() const {
	if (!impObj()) return {};
	return impObj()->originalFileVersion();
}
AcDb::AcDbDwgVersion PyDbDatabase::originalFileSavedByVersion() const {
	if (!impObj()) return {};
	return impObj()->originalFileSavedByVersion();
}
AcDb::MaintenanceReleaseVersion PyDbDatabase::lastSavedAsMaintenanceVersion() const {
	if (!impObj()) return {};
	return impObj()->lastSavedAsMaintenanceVersion();
}
AcDb::MaintenanceReleaseVersion PyDbDatabase::originalFileMaintenanceVersion() const {
	if (!impObj()) return {};
	return impObj()->originalFileMaintenanceVersion();
}
AcDb::MaintenanceReleaseVersion PyDbDatabase::originalFileSavedByMaintenanceVersion() const {
	if (!impObj()) return {};
	return impObj()->originalFileSavedByMaintenanceVersion();
}

AcDwgFileHandle* PyDbDatabase::inputFiler() const {
	return impObj()->inputFiler();
}

PyDbDatabase PyDbDatabase::wblock(const PyDbObjectIdArray& outvals, const AcGePoint3d& basePoint)
{
	AcDbDatabase* pNewDb = nullptr;
	CheckErrorStatus(impObj()->wblock(pNewDb, VecToAcArray<AcDbObjectId>(outvals), basePoint));
	return PyDbDatabase(pNewDb, false);
}

// �汾1��������ID����ͻ��� 
PyDbDatabase PyDbDatabase::wblock(const PyDbObjectIdArray& outvals, const AcGePoint3d& basePoint, AcDb::DuplicateRecordCloning drc)
{
	PyDbDatabase pOutputDb;
	CheckErrorStatus(impObj()->wblock(pOutputDb.impObj(),
		VecToAcArray<AcDbObjectId>(outvals),
		basePoint,
		drc));
	return pOutputDb;
}

// �汾2��ָ����ID
PyDbDatabase PyDbDatabase::wblock(AcDbObjectId blockId)
{
	AcDbDatabase* pNewDb = nullptr;
	CheckErrorStatus(impObj()->wblock(pNewDb, blockId));
	return PyDbDatabase(pNewDb, false);
}

// �汾3�������ݿ� 
PyDbDatabase PyDbDatabase::wblock()
{
	AcDbDatabase* pNewDb = nullptr;
	CheckErrorStatus(impObj()->wblock(pNewDb));
	return PyDbDatabase(pNewDb, false);
}

void PyDbDatabase::forceWblockDatabaseCopy() {
	if (!impObj()) return;
	impObj()->forceWblockDatabaseCopy();
}

// �����������������
AcDbObjectId PyDbDatabase::insert(AcString pBlockName, PyDbDatabase& pDb, bool preserveSourceDatabase)
{
	AcDbObjectId blockId;
	CheckErrorStatus(impObj()->insert(blockId, pBlockName.kwszPtr(), pDb.impObj(), preserveSourceDatabase));
	return blockId;
}

// ������������任����
void PyDbDatabase::insert(const AcGeMatrix3d& xform, PyDbDatabase& pDb, bool preserveSourceDatabase)
{
	CheckErrorStatus(impObj()->insert(xform, pDb.impObj(), preserveSourceDatabase));
}

// �����������Դ/Ŀ�������
AcDbObjectId PyDbDatabase::insert(AcString pSourceBlockName, AcString pDestinationBlockName, PyDbDatabase& pDb, bool preserveSourceDatabase)
{
	AcDbObjectId blockId;
	CheckErrorStatus(impObj()->insert(blockId, pSourceBlockName.kwszPtr(), pDestinationBlockName.kwszPtr(), pDb.impObj(), preserveSourceDatabase));
	return blockId;
}

// ��ȿ�¡���� 
void PyDbDatabase::deepCloneObjects(const PyDbObjectIdArray& objectIds, const AcDbObjectId& owner, AcDbIdMapping& idMap, bool deferXlation) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->deepCloneObjects(VecToAcArray<AcDbObjectId>(objectIds), owner, idMap, deferXlation));
}

// ���¡���� 
void PyDbDatabase::wblockCloneObjects(const PyDbObjectIdArray& objectIds, const AcDbObjectId& owner, AcDbIdMapping& idMap, AcDb::DuplicateRecordCloning drc, bool deferXlation)
{
	CheckErrorStatus(impObj()->wblockCloneObjects(VecToAcArray<AcDbObjectId>(objectIds),
		owner,
		idMap,
		drc,
		deferXlation));
}

// ��ֹ��ȿ�¡
void PyDbDatabase::abortDeepClone(AcDbIdMapping& idMap)
{
	impObj()->abortDeepClone(idMap); // ע�⣺ԭ�����޷���ֵ
}
//
//Acad::ErrorStatus PyDbDatabase::addReactor(PyDbDatabaseReactor& pReactor) const {
//	return impObj()->addReactor(pReactor.impObj());
//}
//
//
//Acad::ErrorStatus PyDbDatabase::removeReactor(PyDbDatabaseReactor& pReactor) const {}
//

// Ӧ�ó���ע����� 
bool PyDbDatabase::registerApp(const AcString& pszAppName) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->registerApp(pszAppName.kwszPtr()));
}

bool PyDbDatabase::isAppRegistered(const AcString& pszAppName) const {
	if (!impObj()) return false;
	return impObj()->isAppRegistered(pszAppName.kwszPtr());
}

// ������� 
Acad::ErrorStatus PyDbDatabase::auditXData(AcDbAuditInfo* pInfo) {
	if (!impObj()) return Acad::eNullObjectPointer;
	return impObj()->auditXData(pInfo);
}

// ����/��������
AcDbUndoController* PyDbDatabase::undoController() const {
	return impObj()->undoController();
}

bool PyDbDatabase::disableUndoRecording(bool disable) {
	if (!impObj()) return false;
	impObj()->disableUndoRecording(disable);
	return true;
}

bool PyDbDatabase::undoRecording() const {
	if (!impObj()) return false;
	return impObj()->undoRecording();
}

// �������
AcDbTransactionManager* PyDbDatabase::transactionManager() const {
	return impObj()->transactionManager();
}

AcDb::MaintenanceReleaseVersion PyDbDatabase::maintenanceReleaseVersion() const
{
	return impObj()->maintenanceReleaseVersion();
}



Acad::ErrorStatus PyDbDatabase::restoreOriginalXrefSymbols() {
	if (!impObj()) return Acad::eNullObjectPointer;
	return impObj()->restoreOriginalXrefSymbols();

}
Acad::ErrorStatus PyDbDatabase::restoreForwardingXrefSymbols() {
	if (!impObj()) return Acad::eNullObjectPointer;
	return impObj()->restoreForwardingXrefSymbols();
}

AcDbObjectId PyDbDatabase::xrefBlockId() const {
	if (!impObj()) return {};
	return impObj()->xrefBlockId();
}


// �ߴ��ע�����Կ��ƣ�DIMASO��
bool PyDbDatabase::dimaso() const {
	if (!impObj()) return false;
	return impObj()->dimaso();
}
bool PyDbDatabase::setDimaso(bool val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setDimaso(val));
}

// �ߴ��עʵʱ���£�DIMSHO��
bool PyDbDatabase::dimsho() const {
	if (!impObj()) return false;
	return impObj()->dimsho();
}
bool PyDbDatabase::setDimsho(bool val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setDimsho(val));
}

// ����߶������ɣ�PLINEGEN��
bool PyDbDatabase::plinegen() const {
	if (!impObj()) return false;
	return impObj()->plinegen();
}
bool PyDbDatabase::setPlinegen(bool val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setPlinegen(val));
}

// ����ģʽ���ƣ�ORTHOMODE��
bool PyDbDatabase::orthomode() const {
	if (!impObj()) return false;
	return impObj()->orthomode();
}
bool PyDbDatabase::setOrthomode(bool val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setOrthomode(val));
}

// ������ģʽ���ƣ�REGENMODE��
bool PyDbDatabase::regenmode() const {
	if (!impObj()) return false;
	return impObj()->regenmode();
}
bool PyDbDatabase::setRegenmode(bool val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setRegenmode(val));
}

// �����ʾ���ƣ�FILLMODE��
bool PyDbDatabase::fillmode() const {
	if (!impObj()) return false;
	return impObj()->fillmode();
}
bool PyDbDatabase::setFillmode(bool val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setFillmode(val));
}

// ��������ģʽ��QTEXTMODE��
bool PyDbDatabase::qtextmode() const {
	if (!impObj()) return false;
	return impObj()->qtextmode();
}
bool PyDbDatabase::setQtextmode(bool val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setQtextmode(val));
}

// ���ͱ������ƣ�PSLTSCALE��
bool PyDbDatabase::psltscale() const {
	if (!impObj()) return false;
	return impObj()->psltscale();
}
bool PyDbDatabase::setPsltscale(bool val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setPsltscale(val));
}

// ��ͼ���Ƽ�飨LIMCHECK��
bool PyDbDatabase::limcheck() const {
	if (!impObj()) return false;
	return impObj()->limcheck();
}
bool PyDbDatabase::setLimcheck(bool val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setLimcheck(val));
}

// ��ʱ�����ʾ��BLIPMODE��
bool PyDbDatabase::blipmode() const {
	if (!impObj()) return false;
	return impObj()->blipmode();
}
bool PyDbDatabase::setBlipmode(bool val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setBlipmode(val));
}

//=== 新增常用系统变量 ===//
bool PyDbDatabase::mirrtext() const { if (!impObj()) return false; return impObj()->mirrtext() != Adesk::kFalse; }
bool PyDbDatabase::setMirrtext(bool val) { if (!impObj()) return false; return CheckErrorStatus(impObj()->setMirrtext(val ? Adesk::kTrue : Adesk::kFalse)); }

int PyDbDatabase::attmode() const { if (!impObj()) return 0; return (int)impObj()->attmode(); }
bool PyDbDatabase::setAttmode(int val) { if (!impObj()) return false; return CheckErrorStatus(impObj()->setAttmode((Adesk::Int16)val)); }


double PyDbDatabase::filletrad() const { if (!impObj()) return 0.0; return impObj()->filletrad(); }
bool PyDbDatabase::setFilletrad(double val) { if (!impObj()) return false; return CheckErrorStatus(impObj()->setFilletrad(val)); }

double PyDbDatabase::chamfera() const { if (!impObj()) return 0.0; return impObj()->chamfera(); }
bool PyDbDatabase::setChamfera(double val) { if (!impObj()) return false; return CheckErrorStatus(impObj()->setChamfera(val)); }

double PyDbDatabase::chamferb() const { if (!impObj()) return 0.0; return impObj()->chamferb(); }
bool PyDbDatabase::setChamferb(double val) { if (!impObj()) return false; return CheckErrorStatus(impObj()->setChamferb(val)); }

double PyDbDatabase::celtscale() const { if (!impObj()) return 0.0; return impObj()->celtscale(); }
bool PyDbDatabase::setCeltscale(double val) { if (!impObj()) return false; return CheckErrorStatus(impObj()->setCeltscale(val)); }

int PyDbDatabase::pdmode() const { if (!impObj()) return 0; return (int)impObj()->pdmode(); }
bool PyDbDatabase::setPdmode(int val) { if (!impObj()) return false; return CheckErrorStatus(impObj()->setPdmode((Adesk::Int16)val)); }

// ����ͼ�α��棨SAVEPROXYGRAPHICS��
Adesk::Int16 PyDbDatabase::saveproxygraphics() const {
	if (!impObj()) return 0;
	return impObj()->saveproxygraphics();
}
void PyDbDatabase::setSaveproxygraphics(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setSaveproxygraphics(val));
}

// ��ɾ����������DELUSEDOBJ��
Adesk::Int16 PyDbDatabase::delUsedObj() const {
	if (!impObj()) return 0;
	return impObj()->delUsedObj();
}
void PyDbDatabase::setDelUsedObj(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setDelUsedObj(val));
}

// ��������ʾ��DISPSILH��
bool PyDbDatabase::dispSilh() const {
	if (!impObj()) return false;
	return impObj()->dispSilh();
}
void PyDbDatabase::setDispSilh(bool val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setDispSilh(val));
}

// �������Բת����PLINEELLIPSE��
bool PyDbDatabase::plineEllipse() const {
	if (!impObj()) return false;
	return impObj()->plineEllipse();
}
void PyDbDatabase::setPlineEllipse(bool val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setPlineEllipse(val));
}

// �û���ʱ��״̬��USRTIMER��
bool PyDbDatabase::usrtimer() const {
	if (!impObj()) return false;
	return impObj()->usrtimer();
}
void PyDbDatabase::setUsrtimer(bool val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setUsrtimer(val));
}

// ��ͼ�����ģʽ��SKPOLY��
bool PyDbDatabase::skpoly() const {
	if (!impObj()) return false;
	return impObj()->skpoly();
}
void PyDbDatabase::setSkpoly(bool val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setSkpoly(val));
}

// �Ƕȷ�����ƣ�ANGDIR��
bool PyDbDatabase::angdir() const {
	if (!impObj()) return false;
	return impObj()->angdir();
}
void PyDbDatabase::setAngdir(bool val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setAngdir(val));
}

// ���������ʾ���ƣ�SPLFRAME��
bool PyDbDatabase::splframe() const {
	if (!impObj()) return false;
	return impObj()->splframe();
}
void PyDbDatabase::setSplframe(bool val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setSplframe(val));
}

// ���ÿ����ʾ���ƣ�XCLIPFRAME��
Adesk::UInt8 PyDbDatabase::xclipFrame() const {
	if (!impObj()) return 0;
	return impObj()->xclipFrame();
}
void PyDbDatabase::setXclipFrame(Adesk::UInt8 val) {
	if (!impObj()) return;
	if (val > 2) throw std::out_of_range("XCLIPFRAME value must be 0-2");
	CheckErrorStatus(impObj()->setXclipFrame(val));
}

// ������ؿ���
bool PyDbDatabase::attreq() const {
	if (!impObj()) return false;
	return impObj()->attreq();
}
void PyDbDatabase::setAttreq(bool val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setAttreq(val));
}

bool PyDbDatabase::attdia() const {
	if (!impObj()) return false;
	return impObj()->attdia();
}
void PyDbDatabase::setAttdia(bool val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setAttdia(val));
}


// ��ͼ��ռ����
bool PyDbDatabase::worldview() const {
	if (!impObj()) return false;
	return impObj()->worldview();
}
void PyDbDatabase::setWorldview(bool val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setWorldview(val));
}

bool PyDbDatabase::tilemode() const {
	if (!impObj()) return false;
	return impObj()->tilemode();
}
bool PyDbDatabase::setTilemode(bool val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setTilemode(val));
}

// ����ϵͳ����
Adesk::Int16 PyDbDatabase::dragmode() const {
	if (!impObj()) return 0;
	return impObj()->dragmode();
}
void PyDbDatabase::setDragmode(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setDragmode(val), L"DRAGMODEֵ����Ϊ0-3");
}


// �ռ�������ȿ��ƣ�TREEDEPTH��
Adesk::Int16 PyDbDatabase::treedepth() const {
	if (!impObj()) return 0;
	return impObj()->treedepth();
}
void PyDbDatabase::setTreedepth(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setTreedepth(val), L"ֵ����Ϊ0-3020");
}

// ���Ե�λ���ƣ�LUNITS/LUPREC��
Adesk::Int16 PyDbDatabase::lunits() const {
	if (!impObj()) return 0;
	return impObj()->lunits();
}
bool PyDbDatabase::setLunits(Adesk::Int16 val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setLunits(val), L"ֵ����Ϊ1-5");
}

Adesk::Int16 PyDbDatabase::luprec() const {
	if (!impObj()) return 0;
	return impObj()->luprec();
}
bool PyDbDatabase::setLuprec(Adesk::Int16 val) {
	if (!impObj()) return false;
	if (val < 0 || val > 8) throw std::out_of_range("luprec value must be 0-8");
	return CheckErrorStatus(impObj()->setLuprec(val));
}

// �Ƕȵ�λ���ƣ�AUNITS/AUPREC��
bool PyDbDatabase::setAunits(Adesk::Int16 val) {
	if (!impObj()) return false;
	if (val < 0 || val > 4) throw std::out_of_range("aunits value must be 0-4");
	return CheckErrorStatus(impObj()->setAunits(val));
}

Adesk::Int16 PyDbDatabase::auprec() const {
	if (!impObj()) return 0;
	return impObj()->auprec();
}
bool PyDbDatabase::setAuprec(Adesk::Int16 val) {
	if (!impObj()) return false;
	if (val < 0 || val > 8) throw std::out_of_range("auprec value must be 0-8");
	return CheckErrorStatus(impObj()->setAuprec(val));
}

Adesk::Int16 PyDbDatabase::pdmode()
{
	return impObj()->pdmode();
}

void PyDbDatabase::setPdmode(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setPdmode(val), L"ֵ����Ϊ0 - 4");
}
// ѡ����ʽ���� 
Adesk::Int16 PyDbDatabase::pickstyle() const {
	if (!impObj()) return 0;
	return impObj()->pickstyle();
}
void PyDbDatabase::setPickstyle(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setPickstyle(val), L"PICKSTYLEֵ����Ϊ0-3");
}

// �û��Զ������ͱ��� 
Adesk::Int16 PyDbDatabase::useri1() const {
	if (!impObj()) return 0;
	return impObj()->useri1();
}
void PyDbDatabase::setUseri1(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setUseri1(val));
}

Adesk::Int16 PyDbDatabase::useri2() const {
	if (!impObj()) return 0;
	return impObj()->useri2();
}
void PyDbDatabase::setUseri2(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setUseri2(val));
}

Adesk::Int16 PyDbDatabase::useri3() const {
	if (!impObj()) return 0;
	return impObj()->useri3();
}
void PyDbDatabase::setUseri3(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setUseri3(val));
}

Adesk::Int16 PyDbDatabase::useri4() const {
	if (!impObj()) return 0;
	return impObj()->useri4();
}
void PyDbDatabase::setUseri4(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setUseri4(val));
}

Adesk::Int16 PyDbDatabase::useri5() const {
	if (!impObj()) return 0;
	return impObj()->useri5();
}
void PyDbDatabase::setUseri5(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setUseri5(val));
}

// �������߿��� 
Adesk::Int16 PyDbDatabase::splinesegs() const {
	if (!impObj()) return 0;
	return impObj()->splinesegs();
}
void PyDbDatabase::setSplinesegs(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setSplinesegs(val), L"SPLINESEGS�������0");
}

// �������
Adesk::Int16 PyDbDatabase::surfu() const {
	if (!impObj()) return 0;
	return impObj()->surfu();
}

void PyDbDatabase::setSurfu(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setSurfu(val), L"SURFUֵ����Ϊ2-200");
}

Adesk::Int16 PyDbDatabase::surfv() const {
	if (!impObj()) return 0;
	return impObj()->surfv();
}
void PyDbDatabase::setSurfv(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setSurfv(val), L"SURFVֵ����Ϊ2-200");
}

Adesk::Int16 PyDbDatabase::surftype() const {
	if (!impObj()) return 0;
	return impObj()->surftype();
}
void PyDbDatabase::setSurftype(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setSurftype(val), L"SURFTYPEֵ����Ϊ5-8");
}

Adesk::Int16 PyDbDatabase::surftab1() const {
	if (!impObj()) return 0;
	return impObj()->surftab1();
}
void PyDbDatabase::setSurftab1(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setSurftab1(val), L"SURFTAB1ֵ����Ϊ2-32766");
}

Adesk::Int16 PyDbDatabase::surftab2() const {
	if (!impObj()) return 0;
	return impObj()->surftab2();
}
void PyDbDatabase::setSurftab2(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setSurftab2(val), L"SURFTAB2ֵ����Ϊ2-32766");
}

Adesk::Int16 PyDbDatabase::splinetype() const {
	if (!impObj()) return 0;
	return impObj()->splinetype();
}
void PyDbDatabase::setSplinetype(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setSplinetype(val), L"SPLINETYPEֵ����Ϊ5��6");
}

Adesk::Int16 PyDbDatabase::shadedge() const {
	if (!impObj()) return 0;
	return impObj()->shadedge();
}
void PyDbDatabase::setShadedge(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setShadedge(val), L"SHADEDGEֵ����Ϊ0-3");
}

Adesk::Int16 PyDbDatabase::shadedif() const {
	if (!impObj()) return 0;
	return impObj()->shadedif();
}
void PyDbDatabase::setShadedif(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setShadedif(val), L"SHADEDIFֵ����Ϊ0-100");
}

// ��λϵͳ���� 
AcDb::MeasurementValue PyDbDatabase::measurement() {
	if (!impObj()) return {};
	return impObj()->measurement();
}

void PyDbDatabase::setMeasurement(AcDb::MeasurementValue val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setMeasurement(val));
}

Adesk::Int16 PyDbDatabase::unitmode() const {
	if (!impObj()) return 0;
	return impObj()->unitmode();
}
void PyDbDatabase::setUnitmode(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setUnitmode(val), L"UNITMODEֵ����Ϊ0��1");
}
// �ӿڿ���
Adesk::Int16 PyDbDatabase::maxactvp() const {
	if (!impObj()) return 0;
	return impObj()->maxactvp();
}
void PyDbDatabase::setMaxactvp(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setMaxactvp(val), L"MAXACTVPֵ����Ϊ1-64");
}

Adesk::Int16 PyDbDatabase::isolines() const {
	if (!impObj()) return 0;
	return impObj()->isolines();
}

void PyDbDatabase::setIsolines(Adesk::Int16 isolines) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setIsolines(isolines));
}

Adesk::UInt8 PyDbDatabase::dwfframe() const {
	if (!impObj()) return 0;
	return impObj()->dwfframe();
}
void PyDbDatabase::setDwfframe(Adesk::UInt8 dwfframe) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setDwfframe(dwfframe));
}

Adesk::UInt8 PyDbDatabase::dgnframe() const {
	if (!impObj()) return 0;
	return impObj()->dgnframe();
}
void PyDbDatabase::setDgnframe(Adesk::UInt8 dgnframe) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setDgnframe(dgnframe));
}

Adesk::Int8 PyDbDatabase::pdfframe() const {
	if (!impObj()) return 0;
	return impObj()->pdfframe();
}
void PyDbDatabase::setPdfframe(Adesk::Int8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setPdfframe(val));
}

double PyDbDatabase::ltscale() const {
	if (!impObj()) return 0.0;
	return impObj()->ltscale();
}
bool PyDbDatabase::setLtscale(double scale) {
	if (!impObj()) return false;
	return CheckErrorStatus((impObj()->setLtscale(scale)));
}

double PyDbDatabase::textsize() const {
	if (!impObj()) return 0.0;
	return impObj()->textsize();
}
bool PyDbDatabase::setTextsize(double size) {
	if (!impObj()) return false;
	return CheckErrorStatus((impObj()->setTextsize(size)));
}

double PyDbDatabase::tracewid() const {
	if (!impObj()) return 0.0;
	return impObj()->tracewid();
}
void PyDbDatabase::setTracewid(double width) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setTracewid(width));
}

double PyDbDatabase::sketchinc() const {
	if (!impObj()) return 0.0;
	return impObj()->sketchinc();
}
void PyDbDatabase::setSketchinc(double inc) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setSketchinc(inc));
}


double PyDbDatabase::thickness() const {
	if (!impObj()) return 0.0;
	return impObj()->thickness();
}
bool PyDbDatabase::setThickness(double thickness) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setThickness(thickness));
}

AcDb::LineWeight PyDbDatabase::celweight() const {
	if (!impObj()) return {};
	return impObj()->celweight();
}
void PyDbDatabase::setCelweight(AcDb::LineWeight weight) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setCelweight(weight));
}

bool PyDbDatabase::isValidLineWeight(int weight) {
	return AcDbDatabase::isValidLineWeight(weight);
}

AcDb::LineWeight PyDbDatabase::getNearestLineWeight(int weight) {
	return AcDbDatabase::getNearestLineWeight(weight);
}

AcDb::EndCaps PyDbDatabase::endCaps() const {
	if (!impObj()) return {};
	return impObj()->endCaps();
}
void PyDbDatabase::setEndCaps(AcDb::EndCaps endCapstype) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setEndCaps(endCapstype));
}

AcDb::JoinStyle PyDbDatabase::joinStyle() const {
	if (!impObj()) return {};
	return impObj()->joinStyle();
}
void PyDbDatabase::setJoinStyle(AcDb::JoinStyle style) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setJoinStyle(style));
}

bool PyDbDatabase::lineWeightDisplay() const {
	if (!impObj()) return false;
	return impObj()->lineWeightDisplay();
}
void PyDbDatabase::setLineWeightDisplay(bool display) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLineWeightDisplay(display));
}

bool PyDbDatabase::xrefEditEnabled() const {
	if (!impObj()) return false;
	return impObj()->xrefEditEnabled();
}
void PyDbDatabase::setXrefEditEnabled(bool enable) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setXrefEditEnabled(enable));
}

bool PyDbDatabase::oleStartUp() const {
	if (!impObj()) return false;
	return impObj()->oleStartUp();
}
void PyDbDatabase::setOleStartUp(bool val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setOleStartUp(val));
}

double PyDbDatabase::angbase() const {
	if (!impObj()) return 0.0;
	return impObj()->angbase();
}
bool PyDbDatabase::setAngbase(double angle) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setAngbase(angle));
}

double PyDbDatabase::pdsize() const {
	if (!impObj()) return 0.0;
	return	impObj()->pdsize();
}
bool PyDbDatabase::setPdsize(double size) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setPdsize(size));
}

double PyDbDatabase::plinewid() const {
	if (!impObj()) return 0.0;
	return impObj()->plinewid();
}
bool PyDbDatabase::setPlinewid(double width) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setPlinewid(width));
}

double PyDbDatabase::userr1() const {
	if (!impObj()) return 0.0;
	return impObj()->userr1();
}
bool PyDbDatabase::setUserr1(double val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setUserr1(val));
}

double PyDbDatabase::userr2() const {
	if (!impObj()) return 0.0;
	return impObj()->userr2();
}
bool PyDbDatabase::setUserr2(double val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setUserr2(val));
}

double PyDbDatabase::userr3() const {
	if (!impObj()) return 0.0;
	return impObj()->userr3();
}
void PyDbDatabase::setUserr3(double val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setUserr3(val));
}
double PyDbDatabase::userr4() const {
	if (!impObj()) return 0.0;
	return impObj()->userr4();
}
void PyDbDatabase::setUserr4(double val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setUserr4(val));
}

double PyDbDatabase::userr5() const {
	if (!impObj()) return 0.0;
	return impObj()->userr5();
}
void PyDbDatabase::setUserr5(double val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setUserr5(val));
}


double PyDbDatabase::chamferc() const {
	if (!impObj()) return 0.0;
	return impObj()->chamferc();
}
void PyDbDatabase::setChamferc(double val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setChamferc(val));
}

double PyDbDatabase::chamferd() const {
	if (!impObj()) return 0.0;
	return impObj()->chamferd();
}
void PyDbDatabase::setChamferd(double val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setChamferd(val));
}

double PyDbDatabase::facetres() const {
	if (!impObj()) return 0.0;
	return	 impObj()->facetres();
}

void PyDbDatabase::setFacetres(double facetres) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setFacetres(facetres));
}

// 3DDWF
double PyDbDatabase::get3dDwfPrec() const {
	if (!impObj()) return 0.0;
	return impObj()->get3dDwfPrec();
}
void PyDbDatabase::set3dDwfPrec(double DwfPrec) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->set3dDwfPrec(DwfPrec));
}

AcString  PyDbDatabase::getMenu() const {
	if (!impObj()) return AcString();
	AcString sMenuName;
	CheckErrorStatus(impObj()->getMenu(sMenuName));
	return sMenuName;
}

bool PyDbDatabase::hpInherit() const {
	if (!impObj()) return false;
	return impObj()->hpInherit();
}
void PyDbDatabase::setHpInherit(const bool inherit) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setHpInherit(inherit));
}

AcGePoint2d PyDbDatabase::hpOrigin() const {
	if (!impObj()) return AcGePoint2d();
	return impObj()->hpOrigin();
}
void PyDbDatabase::setHpOrigin(const AcGePoint2d& origin) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setHpOrigin(origin));
}
// returns a local time
const AcDbDate PyDbDatabase::tdcreate() const {
	if (!impObj()) return AcDbDate();
	return impObj()->tdcreate();
}
const AcDbDate PyDbDatabase::tdupdate() const {
	if (!impObj()) return AcDbDate();
	return impObj()->tdupdate();
}
// returns a universal time
const AcDbDate PyDbDatabase::tducreate() const {
	if (!impObj()) return AcDbDate();
	return impObj()->tducreate();
}
const AcDbDate PyDbDatabase::tduupdate() const {
	if (!impObj()) return AcDbDate();
	return impObj()->tduupdate();
}

// returns a time delta
const AcDbDate PyDbDatabase::tdindwg() const {
	if (!impObj()) return AcDbDate();
	return impObj()->tdindwg();
}
const AcDbDate PyDbDatabase::tdusrtimer() const {
	if (!impObj()) return AcDbDate();
	return impObj()->tdusrtimer();
}

void PyDbDatabase::resetTimes() {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->resetTimes());
}

AcCmColor PyDbDatabase::cecolor() const {
	if (!impObj()) return {};
	return impObj()->cecolor();
}
void PyDbDatabase::setCecolor(const AcCmColor& color) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setCecolor(color));
}

AcCmTransparency PyDbDatabase::cetransparency() const {
	if (!impObj()) return {};
	return impObj()->cetransparency();
}
void PyDbDatabase::setCetransparency(const AcCmTransparency& transparency) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setCetransparency(transparency));
}

AcDbHandle PyDbDatabase::handseed() const {
	if (!impObj()) return {};
	return impObj()->handseed();
}
void PyDbDatabase::setHandseed(const AcDbHandle& handle) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setHandseed(handle));
}

AcDbObjectId PyDbDatabase::clayer() const {
	if (!impObj()) return {};
	return impObj()->clayer();
}

bool PyDbDatabase::setClayer(AcDbObjectId val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setClayer(val));
}

std::tuple<AcDb::PlotStyleNameType, AcDbObjectId> PyDbDatabase::getCePlotStyleNameId() const {
	if (!impObj()) return {};
	AcDbObjectId  val;
	return { impObj()->getCePlotStyleNameId(val),val };
}

void PyDbDatabase::setCePlotStyleName(AcDb::PlotStyleNameType plotsStyleNameType, AcDbObjectId newId) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setCePlotStyleName(plotsStyleNameType, newId));
}

AcDbObjectId PyDbDatabase::textstyle() const {
	if (!impObj()) return {};
	return impObj()->textstyle();
}
bool PyDbDatabase::setTextstyle(AcDbObjectId val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setTextstyle(val));
}

AcDbObjectId PyDbDatabase::celtype() const {
	if (!impObj()) return {};
	return impObj()->celtype();
}
bool PyDbDatabase::setCeltype(AcDbObjectId val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setCeltype(val));
}

AcDbObjectId PyDbDatabase::cmaterial() const {
	if (!impObj()) return {};
	return impObj()->cmaterial();
}
bool PyDbDatabase::setCmaterial(AcDbObjectId val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setCmaterial(val));
}

AcDbObjectId PyDbDatabase::dimstyle() const {
	if (!impObj()) return {};
	return impObj()->dimstyle();
}
bool PyDbDatabase::setDimstyle(AcDbObjectId styleId) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setDimstyle(styleId));
}

// SDK 返回由 SDK 分配的标注样式记录副本（caller owns pRec，用完须 delete）；当前未绑定 Python。
AcDbDimStyleTableRecord* PyDbDatabase::getDimstyleData() const {
	AcDbDimStyleTableRecord* pRec = nullptr;
	CheckErrorStatus(impObj()->getDimstyleData(pRec));
	return	 pRec;
}
// SDK 返回由 SDK 分配的子标注样式记录副本（caller owns pRec，用完须 delete）；当前未绑定 Python。
// 失败时 style 出参内容未定义，调用前先置空，避免把未初始化 ObjectId 暴露给调用方。
AcDbDimStyleTableRecord* PyDbDatabase::getDimstyleChildData(const PyRxClass& pDimClass, AcDbObjectId& style) const {
	AcDbDimStyleTableRecord* pRec = nullptr;
	style = AcDbObjectId::kNull;
	CheckErrorStatus(impObj()->getDimstyleChildData(pDimClass.impObj(), pRec, style));
	return pRec;
}
AcDbObjectId PyDbDatabase::getDimstyleChildId(const PyRxClass& pDimClass, AcDbObjectId& parentStyle) const {
	if (!impObj()) return {};
	return impObj()->getDimstyleChildId(pDimClass.impObj(), parentStyle);
}

AcDbObjectId PyDbDatabase::getDimstyleParentId(AcDbObjectId& childStyle) const {
	if (!impObj()) return {};
	return impObj()->getDimstyleParentId(childStyle);
}

void PyDbDatabase::setDimstyleData(AcDbDimStyleTableRecord* pRec) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setDimstyleData(pRec));
}
void PyDbDatabase::setDimstyleData(AcDbObjectId id) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setDimstyleData(id));
}

AcDbObjectId PyDbDatabase::cmlstyleID() const {
	if (!impObj()) return {};
	return impObj()->cmlstyleID();
}
void PyDbDatabase::setCmlstyleID(AcDbObjectId val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setCmlstyleID(val));
}

Adesk::Int16 PyDbDatabase::cmljust() const {
	if (!impObj()) return 0;
	return impObj()->cmljust();
}
void PyDbDatabase::setCmljust(Adesk::Int16 just) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setCmljust(just));
}

double PyDbDatabase::cmlscale() const {
	if (!impObj()) return 0.0;
	return impObj()->cmlscale();
}
void PyDbDatabase::setCmlscale(double scale) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setCmlscale(scale));
}

//ͳһ�������ģʽ��д���ع�

// Paper space variable access
// ͼֽ�ռ������� 
AcGePoint3d PyDbDatabase::pinsbase() const {
	if (!impObj()) return AcGePoint3d();
	return impObj()->pinsbase();
}
void PyDbDatabase::setPinsbase(const AcGePoint3d& val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setPinsbase(val), L"Invalid paper space base point");
}

// Paper space geometry range (pextmin/pextmax)
AcGePoint3d PyDbDatabase::pextmin() const {
	if (!impObj()) return AcGePoint3d();
	return impObj()->pextmin();
}
AcGePoint3d PyDbDatabase::pextmax() const {
	if (!impObj()) return AcGePoint3d();
	return impObj()->pextmax();
}
void PyDbDatabase::setPextmin(const AcGePoint3d& val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setPextmin(val), L"Invalid paper space min extents");
}
void PyDbDatabase::setPextmax(const AcGePoint3d& val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setPextmax(val), L"Invalid paper space max extents");
}

// Paper space limits (plimmin/plimmax)
AcGePoint2d PyDbDatabase::plimmin() const {
	if (!impObj()) return AcGePoint2d();
	return impObj()->plimmin();
}
AcGePoint2d PyDbDatabase::plimmax() const {
	if (!impObj()) return AcGePoint2d();
	return impObj()->plimmax();
}
void PyDbDatabase::setPlimmin(const AcGePoint2d& val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setPlimmin(val), L"Invalid paper space min limits");
}
void PyDbDatabase::setPlimmax(const AcGePoint2d& val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setPlimmax(val), L"Invalid paper space max limits");
}

// Paper space elevation
double PyDbDatabase::pelevation() const {
	if (!impObj()) return 0.0;
	return impObj()->pelevation();
}
void PyDbDatabase::setPelevation(double val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setPelevation(val), L"Invalid elevation value");
}

// Paper space UCS origin
AcGePoint3d PyDbDatabase::pucsorg() const {
	if (!impObj()) return AcGePoint3d();
	return impObj()->pucsorg();
}

AcGeVector3d PyDbDatabase::pucsxdir() const {
	if (!impObj()) return AcGeVector3d();
	return impObj()->pucsxdir();
}


AcGeVector3d PyDbDatabase::pucsydir() const {
	if (!impObj()) return AcGeVector3d();
	return impObj()->pucsydir();
}

void PyDbDatabase::setPucs(const AcGePoint3d& ucsOrigin, const AcGeVector3d& ucsXDir, const AcGeVector3d& ucsYDir) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setPucs(ucsOrigin, ucsXDir, ucsYDir));
}

AcDbObjectId PyDbDatabase::pucsname() const {
	if (!impObj()) return {};
	return impObj()->pucsname();
}

// ͼֽ�ռ�UCS����
void PyDbDatabase::setPucsname(const AcDbObjectId& val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setPucsname(val),
		L"��Ч��UCS��¼ID");
}

AcDbObjectId PyDbDatabase::pucsBase() const {
	if (!impObj()) return {};
	return AcDbObjectId(impObj()->pucsBase());
}

void PyDbDatabase::setPucsBase(const AcDbObjectId& val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setPucsBase(val),
		L"��Ч�Ļ���UCS ID");
}

// ������ͼ���� 
AcGePoint3d PyDbDatabase::worldPucsBaseOrigin(
	AcDb::OrthographicView val) const {
	return impObj()->worldPucsBaseOrigin(val);
}

void PyDbDatabase::setWorldPucsBaseOrigin(
	const AcGePoint3d& origin,
	AcDb::OrthographicView val) {
	CheckErrorStatus(impObj()->setWorldPucsBaseOrigin(origin, val),
		L"��Ч��������ͼԭ��");
}

bool PyDbDatabase::isPucsOrthographic(
	AcDb::OrthographicView& val) const {
	return impObj()->isPucsOrthographic(val);
}

// ����������� 
double PyDbDatabase::cameraHeight() const {
	if (!impObj()) return 0.0;
	return impObj()->cameraHeight();
}

void PyDbDatabase::setCameraHeight(double val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setCameraHeight(val),
		L"����߶ȱ������0");
}

double PyDbDatabase::lensLength() const {
	if (!impObj()) return 0.0;
	return impObj()->lensLength();
}

void PyDbDatabase::setLensLength(double val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLensLength(val),
		L"��ͷ���ȱ������0");
}

bool PyDbDatabase::cameraDisplay() const {
	if (!impObj()) return false;
	return impObj()->cameraDisplay();
}

void PyDbDatabase::setCameraDisplay(bool val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setCameraDisplay(val));
}

// ������������
double PyDbDatabase::stepsPerSec() const {
	if (!impObj()) return 0.0;
	return impObj()->stepsPerSec();
}
void PyDbDatabase::setStepsPerSec(double val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setStepsPerSec(val), L"������Ƶ�������0");
}

double PyDbDatabase::stepSize() const {
	if (!impObj()) return 0.0;
	return impObj()->stepSize();
}
void PyDbDatabase::setStepSize(double val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setStepSize(val), L"����ֵ�������0");
}

// �������� 
bool PyDbDatabase::realWorldScale() const {
	if (!impObj()) return false;
	return impObj()->realWorldScale();
}
void PyDbDatabase::setRealWorldScale(bool val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setRealWorldScale(val));
}

// ͼ�η�Χ���� 
AcGePoint3d PyDbDatabase::insbase() const {
	if (!impObj()) return AcGePoint3d();
	return impObj()->insbase();
}
void PyDbDatabase::setInsbase(const AcGePoint3d& val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setInsbase(val));
}

AcGePoint3d PyDbDatabase::extmin() const {
	if (!impObj()) return AcGePoint3d();
	return impObj()->extmin();
}
AcGePoint3d PyDbDatabase::extmax() const {
	if (!impObj()) return AcGePoint3d();
	return impObj()->extmax();
}

bool PyDbDatabase::updateExt(bool val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->updateExt(val));
}

void PyDbDatabase::setExtmin(const AcGePoint3d& val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setExtmin(val));
}
void PyDbDatabase::setExtmax(const AcGePoint3d& val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setExtmax(val));
}


AcGePoint2d PyDbDatabase::limmin() const {
	if (!impObj()) return AcGePoint2d();
	return	impObj()->limmin();
}
void PyDbDatabase::setLimmin(const AcGePoint2d& min) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLimmin(min));
}

AcGePoint2d PyDbDatabase::limmax() const {
	if (!impObj()) return AcGePoint2d();
	return	 impObj()->limmax();
}
void PyDbDatabase::setLimmax(const AcGePoint2d& max) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLimmax(max));
}

double PyDbDatabase::elevation() const {
	if (!impObj()) return 0.0;
	return impObj()->elevation();
}
bool PyDbDatabase::setElevation(double elev) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setElevation(elev));
}

AcGePoint3d PyDbDatabase::ucsorg() const {
	if (!impObj()) return AcGePoint3d();
	return impObj()->ucsorg();
}


AcGeVector3d PyDbDatabase::ucsxdir() const {
	if (!impObj()) return AcGeVector3d();
	return impObj()->ucsxdir();
}


AcGeVector3d PyDbDatabase::ucsydir() const {
	if (!impObj()) return AcGeVector3d();
	return impObj()->ucsydir();
}

void PyDbDatabase::setUcs(const AcGePoint3d& ucsOrigin, const AcGeVector3d& ucsXDir, const AcGeVector3d& ucsYDir) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setUcs(ucsOrigin, ucsXDir, ucsYDir));
}

AcDbObjectId PyDbDatabase::ucsname() const {
	if (!impObj()) return {};
	return impObj()->ucsname();
}

void PyDbDatabase::setUcsname(const AcDbObjectId& ucsRecId) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setUcsname(ucsRecId));
}

AcDbObjectId PyDbDatabase::ucsBase() const {
	if (!impObj()) return {};
	return impObj()->ucsBase();
}
void PyDbDatabase::setUcsBase(AcDbObjectId ucsid) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setUcsBase(ucsid));
}

AcGePoint3d PyDbDatabase::worldUcsBaseOrigin(AcDb::OrthographicView orthoView) const {
	if (!impObj()) return AcGePoint3d();
	return impObj()->worldUcsBaseOrigin(orthoView);
}
void PyDbDatabase::setWorldUcsBaseOrigin(const AcGePoint3d& origin, AcDb::OrthographicView orthoView) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setWorldUcsBaseOrigin(origin, orthoView));
}

bool PyDbDatabase::isUcsOrthographic(AcDb::OrthographicView& orthoView) const {
	if (!impObj()) return false;
	return impObj()->isUcsOrthographic(orthoView);
}

bool PyDbDatabase::isEMR() const {
	if (!impObj()) return false;
	return impObj()->isEMR();
}
AcString PyDbDatabase::getDimpost() const {
	if (!impObj()) return AcString();
	return AcString(impObj()->dimpost());
}
AcString PyDbDatabase::getDimapost() const
{
	return AcString(impObj()->dimapost());
}
AcString PyDbDatabase::getDimblk() const
{
	AcString result;
	auto es = impObj()->getDimblk(result);
	if (es != Acad::eOk) return AcString();
	return result;
}

AcString PyDbDatabase::getDimblk1() const
{
	AcString result;
	auto es = impObj()->getDimblk1(result);
	if (es != Acad::eOk) return AcString();//���뱨���ˣ���ӡ��ǰ����λ��
	return result;
}

AcString PyDbDatabase::getDimblk2() const
{
	AcString result;
	auto es = impObj()->getDimblk2(result);
	if (es != Acad::eOk) return AcString();//���뱨���ˣ���ӡ��ǰ����λ��
	return result;
}

int PyDbDatabase::dimfit() const {
	if (!impObj()) return 0;
	return static_cast<int>(impObj()->dimfit());
}
int PyDbDatabase::dimunit() const {
	if (!impObj()) return 0;
	return impObj()->dimunit();
}

void PyDbDatabase::setDimfit(int fit) {
	if (!impObj()) return;
	impObj()->setDimfit(fit);
}
void PyDbDatabase::setDimunit(int unit) {
	if (!impObj()) return;
	impObj()->setDimunit(unit);
}

void PyDbDatabase::getDimRecentStyleList(PyDbObjectIdArray& vals) const {}

void PyDbDatabase::loadLineTypeFile(AcString ltn, AcString filename) {}

AcString PyDbDatabase::getProjectName() const {
	if (!impObj()) return AcString();
	AcString acs;
	CheckErrorStatus(impObj()->getProjectName(acs));
	return acs;
}
bool PyDbDatabase::setProjectName(AcString val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setProjectName(val.kwszPtr()));
}
AcString PyDbDatabase::getHyperlinkBase() const {
	if (!impObj()) return AcString();
	AcString acs;
	CheckErrorStatus(impObj()->getHyperlinkBase(acs));
	return acs;

}

bool PyDbDatabase::setHyperlinkBase(AcString val) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setHyperlinkBase(val.kwszPtr()));
}

AcString PyDbDatabase::getStyleSheet() const {
	if (!impObj()) return AcString();
	AcString acs;
	CheckErrorStatus(impObj()->getStyleSheet(acs));
	return acs;
}

void PyDbDatabase::setStyleSheet(AcString val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setStyleSheet(val.kwszPtr()));
}


AcString PyDbDatabase::getFilename() const {
	if (!impObj()) return AcString();
	const ACHAR* pName = nullptr;
	Acad::ErrorStatus es = impObj()->getFilename(pName);
	if (es != Acad::eOk || !pName) {
		PyWarnIfError(es, L"AcDbDatabase::getFilename");
		return AcString();
	}
	return AcString(pName);
}

bool PyDbDatabase::isPartiallyOpened() const {
	if (!impObj()) return false;
	return	 impObj()->isPartiallyOpened();
}

void PyDbDatabase::applyPartialOpenFilters(const AcDbSpatialFilter* pSpatialFilter, const AcDbLayerFilter* pLayerFilter) {
	if (!impObj()) return;
	impObj()->applyPartialOpenFilters(pSpatialFilter, pLayerFilter);
}

void PyDbDatabase::disablePartialOpen() {
	if (!impObj()) return;
	impObj()->disablePartialOpen();
}

AcString PyDbDatabase::getFingerprintGuid() const {
	if (!impObj()) return AcString();
	AcString acs;
	CheckErrorStatus(impObj()->getFingerprintGuid(acs));
	return acs;
}


void PyDbDatabase::setFingerprintGuid(AcString val) {
	if (!impObj()) return;
	impObj()->setFingerprintGuid(val.kwszPtr());
}

AcString  PyDbDatabase::getVersionGuid() const {
	if (!impObj()) return AcString();
	AcString acs;
	CheckErrorStatus(impObj()->getVersionGuid(acs));
	return acs;
}

void PyDbDatabase::setVersionGuid(AcString val) {
	if (!impObj()) return;
	impObj()->setVersionGuid(val.kwszPtr());

}

int PyDbDatabase::tstackalign() const {
	if (!impObj()) return 0;
	return impObj()->tstackalign();
}

void PyDbDatabase::setTStackAlign(int val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setTStackAlign(val));
}

int PyDbDatabase::tstacksize() const {
	if (!impObj()) return 0;
	return impObj()->tstacksize();
}
void PyDbDatabase::setTStackSize(int val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setTStackSize(val));
}

AcDb::UnitsValue PyDbDatabase::insunits() const {
	if (!impObj()) return {};
	return impObj()->insunits();
}
bool PyDbDatabase::setInsunits(const AcDb::UnitsValue units) {
	if (!impObj()) return false;
	return CheckErrorStatus(impObj()->setInsunits(units));
}

bool PyDbDatabase::plotStyleMode() const {
	if (!impObj()) return false;
	return impObj()->plotStyleMode();
}

// Default paperspace viewport scale
double PyDbDatabase::viewportScaleDefault() const {
	if (!impObj()) return 0.0;
	return impObj()->viewportScaleDefault();
}
void PyDbDatabase::setViewportScaleDefault(double val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setViewportScaleDefault(val));
}

// dwg watermark
bool PyDbDatabase::dwgFileWasSavedByAutodeskSoftware() const {
	if (!impObj()) return false;
	return impObj()->dwgFileWasSavedByAutodeskSoftware();
}

// Layer state manager API
AcDbLayerStateManager* PyDbDatabase::getLayerStateManager() const {
	return	 impObj()->getLayerStateManager();
}

// Object context manager API
AcDbObjectContextManager* PyDbDatabase::objectContextManager() const {
	return	 impObj()->objectContextManager();
}

// When entity traversals are sorted (AcDb::kSortEnts*).
Adesk::UInt8 PyDbDatabase::sortEnts() const {
	if (!impObj()) return 0;
	return	 impObj()->sortEnts();

}
void PyDbDatabase::setSortEnts(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setSortEnts(val));
}

Adesk::UInt8 PyDbDatabase::drawOrderCtl() const {
	if (!impObj()) return 0;
	return	 impObj()->drawOrderCtl();
}
void PyDbDatabase::setDrawOrderCtl(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setDrawOrderCtl(val));
}

Adesk::UInt8 PyDbDatabase::dimAssoc() const {
	if (!impObj()) return 0;
	return	 impObj()->dimAssoc();
}

void PyDbDatabase::setDimAssoc(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setDimAssoc(val));
}

Adesk::UInt8 PyDbDatabase::hideText() const {
	if (!impObj()) return 0;
	return	 impObj()->hideText();
}
void PyDbDatabase::setHideText(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setHideText(val));
}

Adesk::UInt8 PyDbDatabase::haloGap() const {
	if (!impObj()) return 0;
	return	 impObj()->haloGap();
}

void PyDbDatabase::setHaloGap(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setHaloGap(val));

}

Adesk::UInt16 PyDbDatabase::obscuredColor() const {
	if (!impObj()) return 0;
	return	 impObj()->obscuredColor();
}
void PyDbDatabase::setObscuredColor(Adesk::UInt16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setObscuredColor(val));
}

Adesk::UInt8 PyDbDatabase::obscuredLineType() const {
	if (!impObj()) return 0;
	return	 impObj()->obscuredLineType();
}
void PyDbDatabase::setObscuredLineType(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setObscuredLineType(val));
}

Adesk::UInt8 PyDbDatabase::intersectDisplay() const {
	if (!impObj()) return 0;
	return	 impObj()->intersectDisplay();
}

void PyDbDatabase::setIntersectDisplay(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setIntersectDisplay(val));
}

Adesk::UInt16 PyDbDatabase::intersectColor() const {
	if (!impObj()) return 0;
	return	 impObj()->intersectColor();
}
void PyDbDatabase::setIntersectColor(Adesk::UInt16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setIntersectColor(val));
}

// For the ACAD_TABLESTYLE dictionary
void PyDbDatabase::getTableStyleDictionary(AcDbDictionary*& pDict, AcDb::OpenMode mode) {
	if (!impObj()) return;
	impObj()->getTableStyleDictionary(pDict, mode);
}

AcDbObjectId PyDbDatabase::tableStyleDictionaryId() const {
	if (!impObj()) return {};
	return impObj()->tableStyleDictionaryId();
}


// The following functions are similar to setDimstyle/dimstyle
//
AcDbObjectId PyDbDatabase::tablestyle() const {
	if (!impObj()) return {};
	return	 impObj()->tablestyle();
}
void PyDbDatabase::setTablestyle(AcDbObjectId val) {
	if (!impObj()) return;
	impObj()->setTablestyle(val);
}

// For the ACAD_MLEADERSTYLE dictionary
AcDbDictionary* PyDbDatabase::getMLeaderStyleDictionary(AcDb::OpenMode mode = AcDb::kForRead) {
	AcDbDictionary* pDict = nullptr;
	CheckErrorStatus(impObj()->getMLeaderStyleDictionary(pDict, mode));
	return pDict;
}
AcDbObjectId PyDbDatabase::mleaderStyleDictionaryId() const {
	if (!impObj()) return {};
	return impObj()->mleaderStyleDictionaryId();
}



// The following functions are similar to setDimstyle/dimstyle
//
AcDbObjectId PyDbDatabase::mleaderstyle() const {
	if (!impObj()) return {};
	return impObj()->mleaderstyle();
}
void PyDbDatabase::setMLeaderstyle(AcDbObjectId val) {
	if (!impObj()) return;
	impObj()->setMLeaderstyle(val);
}

// For the ACAD_DETAILVIEWSTYLE dictionary
AcDbDictionary* PyDbDatabase::getDetailViewStyleDictionary(AcDb::OpenMode mode = AcDb::kForRead) {
	AcDbDictionary* pDict = nullptr;
	CheckErrorStatus(impObj()->getDetailViewStyleDictionary(pDict, mode));
	return pDict;
}
AcDbObjectId PyDbDatabase::detailViewStyleDictionaryId() const {
	if (!impObj()) return {};
	return	impObj()->detailViewStyleDictionaryId();
}
AcDbObjectId PyDbDatabase::detailViewStyle() const {
	if (!impObj()) return {};
	return	impObj()->detailViewStyle();
}
void PyDbDatabase::setDetailViewStyle(AcDbObjectId val)
{
	CheckErrorStatus(impObj()->setDetailViewStyle(val));
}
//Do not change format of setDetailViewStyle function! 
//We need to preserve format which SymbolTableInfo struct defines for AcDiObservableCollection datatype (AcDiImpManager object)

// For the ACAD_SECTIONVIEWSTYLE dictionary
AcDbDictionary* PyDbDatabase::getSectionViewStyleDictionary(AcDb::OpenMode mode = AcDb::kForRead) {
	AcDbDictionary* pDict = nullptr;
	CheckErrorStatus(impObj()->getSectionViewStyleDictionary(pDict, mode));
	return pDict;
}

AcDbObjectId  PyDbDatabase::sectionViewStyleDictionaryId() const {
	if (!impObj()) return {};
	return impObj()->sectionViewStyleDictionaryId();
}

AcDbObjectId PyDbDatabase::sectionViewStyle() const {
	if (!impObj()) return {};
	return impObj()->sectionViewStyle();
}
//Do not change format of setSectionViewStyle function! 
//We need to preserve format which SymbolTableInfo struct defines for AcDiObservableCollection datatype (AcDiImpManager object)
void PyDbDatabase::setSectionViewStyle(AcDbObjectId val) {
	if (!impObj()) return;
	impObj()->setSectionViewStyle(val);
}

// Get all AcDbViewports in all AcDbLayouts in the database
PyDbObjectIdArray PyDbDatabase::getViewportArray(bool bGetPaperspaceVports = true) const {
	if (!impObj()) return {};

	AcDbObjectIdArray arr;
	if (CheckErrorStatus(impObj()->getViewportArray(arr, bGetPaperspaceVports))) {
		return	 AcArrayToVec<AcDbObjectId>(arr);
	}
	return 	PyDbObjectIdArray();
}

// Get names of all visual styles in the database (visual styles for
// internal use only are excluded)
std::vector<AcString> PyDbDatabase::getVisualStyleList() {
	if (!impObj()) return {};
	AcArray<const wchar_t*> arr;
	if (CheckErrorStatus(impObj()->getVisualStyleList(arr))) {
		return	 AcArrayToVec<AcString>(arr);
	}
	return std::vector<AcString>();
}

Adesk::UInt8 PyDbDatabase::solidHist() const {
	if (!impObj()) return 0;
	return	 impObj()->solidHist();
} // 0 (default) or 1
void PyDbDatabase::setSolidHist(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setSolidHist(val));
}
// 0, 1(default) or 2
Adesk::UInt8 PyDbDatabase::showHist() const {
	if (!impObj()) return 0;
	return	 impObj()->showHist();
}
void PyDbDatabase::setShowHist(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setShowHist(val));
}

double PyDbDatabase::psolWidth() const {
	if (!impObj()) return 0.0;
	return	 impObj()->psolWidth();
}
void PyDbDatabase::setPsolWidth(double width) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setPsolWidth(width));
}
double PyDbDatabase::psolHeight() const {
	if (!impObj()) return 0.0;
	return	 impObj()->psolHeight();
}
void PyDbDatabase::setPsolHeight(double height) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setPsolHeight(height));
}

Adesk::UInt16 PyDbDatabase::loftParam() const {
	if (!impObj()) return 0;
	return	 impObj()->loftParam();
}

void PyDbDatabase::setLoftParam(Adesk::UInt16 flags) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLoftParam(flags));
}

Adesk::UInt8 PyDbDatabase::loftNormals() const {
	if (!impObj()) return 0;
	return	 impObj()->loftNormals();
}
void PyDbDatabase::setLoftNormals(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLoftNormals(val));
}
double PyDbDatabase::loftAng1() const {
	if (!impObj()) return 0.0;
	return	 impObj()->loftAng1();
}
void PyDbDatabase::setLoftAng1(double ang1) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLoftAng1(ang1));
}
double PyDbDatabase::loftAng2() const {
	if (!impObj()) return 0.0;
	return	 impObj()->loftAng2();
}
void PyDbDatabase::setLoftAng2(double ang2) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLoftAng2(ang2));
}
double PyDbDatabase::loftMag1() const {
	if (!impObj()) return 0.0;
	return	 impObj()->loftMag1();
}
void PyDbDatabase::setLoftMag1(double mag1) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLoftMag1(mag1));
}
double PyDbDatabase::loftMag2() const {
	if (!impObj()) return 0.0;
	return	 impObj()->loftMag2();
}
void PyDbDatabase::setLoftMag2(double mag2) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLoftMag2(mag2));
}

// Lat and long are in decimal degrees, not radians!
double PyDbDatabase::latitude() const {
	if (!impObj()) return 0.0;
	return	 impObj()->latitude();
}

void PyDbDatabase::setLatitude(double lat) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLatitude(lat));
}
double PyDbDatabase::longitude() const {
	if (!impObj()) return 0.0;
	return	 impObj()->longitude();
}
void PyDbDatabase::setLongitude(double lng) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLongitude(lng));
}
double PyDbDatabase::northDirection() const {
	if (!impObj()) return 0.0;
	return	 impObj()->northDirection();
}

void PyDbDatabase::setNorthDirection(double northdir) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setNorthDirection(northdir));
}
AcDb::TimeZone PyDbDatabase::timeZone() const {
	if (!impObj()) return {};
	return	 impObj()->timeZone();
}
void PyDbDatabase::setTimeZone(AcDb::TimeZone tz) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setTimeZone(tz));
}
// Offsets measured in hours difference from UTC
void PyDbDatabase::getTimeZoneInfo(AcDb::TimeZone tz, double& offset, AcString& desc) const {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->getTimeZoneInfo(tz, offset, desc));
}
void PyDbDatabase::setTimeZoneAsUtcOffset(double offset) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setTimeZoneAsUtcOffset(offset));
}
bool PyDbDatabase::geoMarkerVisibility() const {
	if (!impObj()) return false;
	return	 impObj()->geoMarkerVisibility();
}
void PyDbDatabase::setGeoMarkerVisibility(bool value) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setGeoMarkerVisibility(value));
}
AcString PyDbDatabase::geoCoordinateSystemId() const {
	if (!impObj()) return AcString();
	return	 impObj()->geoCoordinateSystemId();
}

Adesk::UInt8 PyDbDatabase::lightGlyphDisplay() const { // 0 or 1(default)
	if (!impObj()) return 0;
	return	 impObj()->lightGlyphDisplay();
}
void PyDbDatabase::setLightGlyphDisplay(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLightGlyphDisplay(val));
}
Adesk::UInt8 PyDbDatabase::tileModeLightSynch() const { // 0 or 1(default)
	if (!impObj()) return 0;
	return	 impObj()->tileModeLightSynch();
}
void PyDbDatabase::setTileModeLightSynch(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setTileModeLightSynch(val));
}

AcCmColor PyDbDatabase::interfereColor() const {
	if (!impObj()) return {};
	return	 impObj()->interfereColor();
}
void  PyDbDatabase::setInterfereColor(const AcCmColor& color) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setInterfereColor(color));
}

AcDbObjectId PyDbDatabase::interfereObjVisStyle() const {
	if (!impObj()) return {};
	return	 impObj()->interfereObjVisStyle();
}
void  PyDbDatabase::setInterfereObjVisStyle(AcDbObjectId id) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setInterfereObjVisStyle(id));
}

AcDbObjectId  PyDbDatabase::interfereVpVisStyle() const {
	if (!impObj()) return {};
	return	 impObj()->interfereVpVisStyle();
}

void  PyDbDatabase::setInterfereVpVisStyle(AcDbObjectId id) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setInterfereVpVisStyle(id));
}

AcDbObjectId PyDbDatabase::dragVisStyle() const {
	if (!impObj()) return {};
	return	 impObj()->dragVisStyle();
}
void PyDbDatabase::setDragVisStyle(AcDbObjectId id) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setDragVisStyle(id));
}

Adesk::UInt8 PyDbDatabase::cshadow() const {
	if (!impObj()) return 0;
	return	 impObj()->cshadow();
}
void PyDbDatabase::setCshadow(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setCshadow(val));
}


double PyDbDatabase::shadowPlaneLocation() const {
	if (!impObj()) return 0.0;
	return	 impObj()->shadowPlaneLocation();
}
void  PyDbDatabase::setShadowPlaneLocation(double val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setShadowPlaneLocation(val));
}

AcDbAnnotationScale* PyDbDatabase::cannoscale() const {
	return	 impObj()->cannoscale();
}
void  PyDbDatabase::setCannoscale(AcDbAnnotationScale* pScale) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setCannoscale(pScale));
}

bool  PyDbDatabase::annoAllVisible() const {
	if (!impObj()) return false;
	return	 impObj()->annoAllVisible();
}
void PyDbDatabase::setAnnoAllVisible(bool allvis) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setAnnoAllVisible(allvis));
}

bool  PyDbDatabase::annotativeDwg() const {
	if (!impObj()) return false;
	return	 impObj()->annotativeDwg();
}
void  PyDbDatabase::setAnnotativeDwg(bool anno) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setAnnotativeDwg(anno));
}

bool  PyDbDatabase::msltscale() const {
	if (!impObj()) return false;
	return	 impObj()->msltscale();
}
void  PyDbDatabase::setMsltscale(bool val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setMsltscale(val));
}

Adesk::UInt8  PyDbDatabase::lightingUnits() const {
	if (!impObj()) return 0;
	return	 impObj()->lightingUnits();
}
void  PyDbDatabase::setLightingUnits(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLightingUnits(val));
}

bool  PyDbDatabase::lightsInBlocks() const {
	if (!impObj()) return false;
	return	 impObj()->lightsInBlocks();
}
void  PyDbDatabase::setLightsInBlocks(bool val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLightsInBlocks(val));
}

Adesk::Int16  PyDbDatabase::dxeVal() const {
	if (!impObj()) return 0;
	return	 impObj()->dxeVal();
}
void  PyDbDatabase::setDxeVal(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setDxeVal(val));
}

double  PyDbDatabase::mleaderscale() const {
	if (!impObj()) return 0.0;
	return	 impObj()->mleaderscale();
}
void  PyDbDatabase::setMLeaderscale(double scale) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setMLeaderscale(scale));
}

Adesk::Int16  PyDbDatabase::cDynDisplayMode() const {
	if (!impObj()) return 0;
	return	 impObj()->cDynDisplayMode();
}
void  PyDbDatabase::setCDynDisplayMode(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setCDynDisplayMode(val));
}

Adesk::Int16  PyDbDatabase::previewType() const {
	if (!impObj()) return 0;
	return	 impObj()->previewType();
}
void  PyDbDatabase::setPreviewType(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setPreviewType(val));
}

Adesk::UInt8  PyDbDatabase::indexctl() const {
	if (!impObj()) return 0;
	return	 impObj()->indexctl();
}
void  PyDbDatabase::setIndexctl(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setIndexctl(val));
}

Adesk::UInt8  PyDbDatabase::layerEval() const {
	if (!impObj()) return 0;
	return	 impObj()->layerEval();
}
void  PyDbDatabase::setLayerEval(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLayerEval(val));
}

Adesk::Int16  PyDbDatabase::layerNotify() const {
	if (!impObj()) return 0;
	return	 impObj()->layerNotify();
}
void  PyDbDatabase::setLayerNotify(Adesk::Int16 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setLayerNotify(val));
}

double  PyDbDatabase::msOleScale() const {
	if (!impObj()) return 0.0;
	return	 impObj()->msOleScale();
}
void  PyDbDatabase::setMsOleScale(double val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setMsOleScale(val));
}

Adesk::UInt8 PyDbDatabase::updateThumbnail() const {
	if (!impObj()) return 0;
	return	 impObj()->updateThumbnail();
}
void  PyDbDatabase::setUpdateThumbnail(Adesk::UInt8 val) {
	if (!impObj()) return;
	CheckErrorStatus(impObj()->setUpdateThumbnail(val));
}

Adesk::UInt32  PyDbDatabase::eraseEmptyObjects(const Adesk::UInt32 flags) {
	if (!impObj()) return 0;
	return	 impObj()->eraseEmptyObjects(flags);
}
Adesk::UInt32  PyDbDatabase::countEmptyObjects(const Adesk::UInt32 flags) {
	if (!impObj()) return 0;
	return	 impObj()->countEmptyObjects(flags);
}




PyDbObjectIdArray PyDbTextFind(PyDbDatabase& pDatabase, PyDbObjectIdArray& resultSet, AcString findString, AcString replaceString, FindTextOptions searchOptions, const PyDbObjectIdArray& selSet)
{
	AcDbObjectIdArray arr;
	AcDbObjectIdArray acSelset = VecToAcArray<AcDbObjectId>(selSet);
	bool isFind = acdbTextFind(pDatabase.impObj(), arr, findString.kwszPtr(), replaceString.kwszPtr(), searchOptions, acSelset);
	if (isFind) {
		return AcArrayToVec<AcDbObjectId>(arr);
	}
	return PyDbObjectIdArray{};
}

bool IsGsWorkerThread()
{
	return  acdbIsGsWorkerThread();
}

void bind_PyDbDatabase(pym& m)
{
	py::class_<PyDbDatabase, PyRxObject>(m, "PyDbDatabase")
		.def(py::init([]() { return new PyDbDatabase(new AcDbDatabase(false, true), true); }), "Creates an empty in-memory database")
		.def(py::init<bool, bool>(), py::arg("buildDefaultDrawing"), py::arg("noDocument"),
			"Creates a database with/without a default drawing and document")
		PyWrapper_BIND(DbDatabase)
		// ---- persistent ids exposed as uint64 (asOldId); 0 when null ----
		.def("blockTableId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.blockTableId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("layerTableId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.layerTableId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("textStyleTableId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.textStyleTableId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("linetypeTableId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.linetypeTableId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("viewTableId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.viewTableId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("ucsTableId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.UCSTableId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("viewportTableId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.viewportTableId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("regAppTableId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.regAppTableId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("dimStyleTableId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.dimStyleTableId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("namedObjectsDictionaryId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.namedObjectsDictionaryId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("currentSpaceId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.currentSpaceId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("paperSpaceVportId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.paperSpaceVportId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("layerZero", [](const PyDbDatabase& d) -> uint64_t { auto id = d.layerZero(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("continuousLinetype", [](const PyDbDatabase& d) -> uint64_t { auto id = d.continuousLinetype(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("byLayerLinetype", [](const PyDbDatabase& d) -> uint64_t { auto id = d.byLayerLinetype(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("byBlockLinetype", [](const PyDbDatabase& d) -> uint64_t { auto id = d.byBlockLinetype(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("globalMaterial", [](const PyDbDatabase& d) -> uint64_t { auto id = d.globalMaterial(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("byLayerMaterial", [](const PyDbDatabase& d) -> uint64_t { auto id = d.byLayerMaterial(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("byBlockMaterial", [](const PyDbDatabase& d) -> uint64_t { auto id = d.byBlockMaterial(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })

		// ---- current entity settings: ids as uint64 (setters take ObjectId, left skeleton) ----
		.def("clayer", [](const PyDbDatabase& d) -> uint64_t { auto id = d.clayer(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("textstyle", [](const PyDbDatabase& d) -> uint64_t { auto id = d.textstyle(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("celtype", [](const PyDbDatabase& d) -> uint64_t { auto id = d.celtype(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("cmaterial", [](const PyDbDatabase& d) -> uint64_t { auto id = d.cmaterial(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("dimstyle", [](const PyDbDatabase& d) -> uint64_t { auto id = d.dimstyle(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })

		// ---- 当前实体默认设置 setter（ObjectId 以 uint64 入参；成功返回 true） ----
		.def("setClayer", [](PyDbDatabase& d, uint64_t objId) -> bool {
			AcDbObjectId id; id.setFromOldId(objId);
			return d.setClayer(id);
		}, py::arg("layerId"), "设置当前图层；成功返回 true")
		.def("setTextstyle", [](PyDbDatabase& d, uint64_t objId) -> bool {
			AcDbObjectId id; id.setFromOldId(objId);
			return d.setTextstyle(id);
		}, py::arg("textStyleId"), "设置当前文字样式；成功返回 true")
		.def("setCeltype", [](PyDbDatabase& d, uint64_t objId) -> bool {
			AcDbObjectId id; id.setFromOldId(objId);
			return d.setCeltype(id);
		}, py::arg("linetypeId"), "设置当前线型；成功返回 true")
		.def("setCmaterial", [](PyDbDatabase& d, uint64_t objId) -> bool {
			AcDbObjectId id; id.setFromOldId(objId);
			return d.setCmaterial(id);
		}, py::arg("materialId"), "设置当前材质；成功返回 true")
		.def("setDimstyle", [](PyDbDatabase& d, uint64_t styleId) -> bool {
			AcDbObjectId id; id.setFromOldId(styleId);
			return d.setDimstyle(id);
		}, py::arg("dimStyleId"), "设置当前标注样式；成功返回 true")

		// ---- scalar system variables (safe no-arg / simple-arg) ----
		.def("ltscale", &PyDbDatabase::ltscale)
		.def("setLtscale", &PyDbDatabase::setLtscale, py::arg("scale"))
		.def("textsize", &PyDbDatabase::textsize)
		.def("setTextsize", &PyDbDatabase::setTextsize, py::arg("size"))
		.def("thickness", &PyDbDatabase::thickness)
		.def("setThickness", &PyDbDatabase::setThickness, py::arg("thickness"))
		.def("elevation", &PyDbDatabase::elevation)
		.def("setElevation", &PyDbDatabase::setElevation, py::arg("elev"))
		.def("lunits", &PyDbDatabase::lunits)
		.def("setLunits", &PyDbDatabase::setLunits, py::arg("val"))
		.def("luprec", &PyDbDatabase::luprec)
		.def("setLuprec", &PyDbDatabase::setLuprec, py::arg("val"))
		.def("aunits", &PyDbDatabase::aunits)
		.def("setAunits", &PyDbDatabase::setAunits, py::arg("val"))
		.def("auprec", &PyDbDatabase::auprec)
		.def("setAuprec", &PyDbDatabase::setAuprec, py::arg("val"))
		.def("dimaso", &PyDbDatabase::dimaso)
		.def("setDimaso", &PyDbDatabase::setDimaso, py::arg("val"))
		.def("dimsho", &PyDbDatabase::dimsho)
		.def("setDimsho", &PyDbDatabase::setDimsho, py::arg("val"))
		.def("fillmode", &PyDbDatabase::fillmode)
		.def("setFillmode", &PyDbDatabase::setFillmode, py::arg("val"))
		.def("limcheck", &PyDbDatabase::limcheck)
		.def("setLimcheck", &PyDbDatabase::setLimcheck, py::arg("val"))
		.def("psltscale", &PyDbDatabase::psltscale)
		.def("setPsltscale", &PyDbDatabase::setPsltscale, py::arg("val"))
		.def("tilemode", &PyDbDatabase::tilemode)
		.def("setTilemode", &PyDbDatabase::setTilemode, py::arg("val"))
		.def("orthomode", &PyDbDatabase::orthomode)
		.def("setOrthomode", &PyDbDatabase::setOrthomode, py::arg("val"))
		.def("insunits", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.insunits()); })
		.def("setInsunits", [](PyDbDatabase& d, int units) -> bool {
			return d.setInsunits(static_cast<AcDb::UnitsValue>(units));
		}, py::arg("units"), "设置插入单位；成功返回 true")
		.def("approxNumObjects", &PyDbDatabase::approxNumObjects)
		.def("numberOfSaves", &PyDbDatabase::numberOfSaves)
		.def("originalFileName", &PyDbDatabase::originalFileName)
		.def("isPartiallyOpened", &PyDbDatabase::isPartiallyOpened)
		.def("plotStyleMode", &PyDbDatabase::plotStyleMode)
		.def("viewportScaleDefault", &PyDbDatabase::viewportScaleDefault)

		// ---- additional scalar system variables (correct passthroughs already implemented) ----
		.def("plinegen", &PyDbDatabase::plinegen)
		.def("setPlinegen", &PyDbDatabase::setPlinegen, py::arg("val"))
		.def("regenmode", &PyDbDatabase::regenmode)
		.def("setRegenmode", &PyDbDatabase::setRegenmode, py::arg("val"))
		.def("qtextmode", &PyDbDatabase::qtextmode)
		.def("setQtextmode", &PyDbDatabase::setQtextmode, py::arg("val"))
		.def("blipmode", &PyDbDatabase::blipmode)
		.def("setBlipmode", &PyDbDatabase::setBlipmode, py::arg("val"))
		// === 新增常用系统变量 ===
		.def("mirrtext", &PyDbDatabase::mirrtext)
		.def("setMirrtext", &PyDbDatabase::setMirrtext, py::arg("val"))
		.def("attmode", &PyDbDatabase::attmode)
		.def("setAttmode", &PyDbDatabase::setAttmode, py::arg("val"))
		.def("filletrad", &PyDbDatabase::filletrad)
		.def("setFilletrad", &PyDbDatabase::setFilletrad, py::arg("val"))
		.def("chamfera", &PyDbDatabase::chamfera)
		.def("setChamfera", &PyDbDatabase::setChamfera, py::arg("val"))
		.def("chamferb", &PyDbDatabase::chamferb)
		.def("setChamferb", &PyDbDatabase::setChamferb, py::arg("val"))
		// PyRx 对齐：系统变量零参读取（走统一 getSysVar，类型由 resbuf 分支给出 int/float/str）
		// 注：angdir/chamferc/chamferd/cmljust/cmlscale/dimfit/dimunit 已由下方 impObj 直连绑定，此处不再重复注册
		.def("celweight", [](PyDbDatabase& d){ return d.getSysVar("CELWEIGHT"); })
		.def("dimassoc", [](PyDbDatabase& d){ return d.getSysVar("DIMASSOC"); })
		.def("celtscale", &PyDbDatabase::celtscale)
		.def("setCeltscale", &PyDbDatabase::setCeltscale, py::arg("val"))
		.def("angbase", &PyDbDatabase::angbase)
		.def("setAngbase", &PyDbDatabase::setAngbase, py::arg("angle"))
		.def("pdsize", &PyDbDatabase::pdsize)
		.def("setPdsize", &PyDbDatabase::setPdsize, py::arg("size"))
		.def("plinewid", &PyDbDatabase::plinewid)
		.def("setPlinewid", &PyDbDatabase::setPlinewid, py::arg("width"))
		.def("userr1", &PyDbDatabase::userr1)
		.def("setUserr1", &PyDbDatabase::setUserr1, py::arg("val"))
		.def("userr2", &PyDbDatabase::userr2)
		.def("setUserr2", &PyDbDatabase::setUserr2, py::arg("val"))
		.def("undoRecording", &PyDbDatabase::undoRecording)
		.def("disableUndoRecording", &PyDbDatabase::disableUndoRecording, py::arg("disable"))
		.def("isEMR", &PyDbDatabase::isEMR)
		.def("getMenu", &PyDbDatabase::getMenu)
		.def("getProjectName", &PyDbDatabase::getProjectName)
		.def("setProjectName", &PyDbDatabase::setProjectName, py::arg("val"))
		.def("getHyperlinkBase", &PyDbDatabase::getHyperlinkBase)
		.def("setHyperlinkBase", &PyDbDatabase::setHyperlinkBase, py::arg("val"))
		.def("getDimpost", &PyDbDatabase::getDimpost)
		.def("getDimapost", &PyDbDatabase::getDimapost)
		.def("getDimblk", &PyDbDatabase::getDimblk)
		.def("getDimblk1", &PyDbDatabase::getDimblk1)
		.def("getDimblk2", &PyDbDatabase::getDimblk2)
		.def("dwgFileWasSavedByAutodeskSoftware", &PyDbDatabase::dwgFileWasSavedByAutodeskSoftware)
		.def("getVisualStyleList", [](PyDbDatabase& d) { return d.getVisualStyleList(); })
		.def("isValidLineWeight", &PyDbDatabase::isValidLineWeight, py::arg("weight"))
		.def("getNearestLineWeight", [](PyDbDatabase& d, int w) -> int {
			return static_cast<int>(d.getNearestLineWeight(w)); }, py::arg("weight"))
		// ---- database timestamps (wrapped as PyDbDate) ----
		.def("tdcreate", [](const PyDbDatabase& d) { return PyDbDate(d.tdcreate()); })
		.def("tdupdate", [](const PyDbDatabase& d) { return PyDbDate(d.tdupdate()); })
		.def("tducreate", [](const PyDbDatabase& d) { return PyDbDate(d.tducreate()); })
		.def("tduupdate", [](const PyDbDatabase& d) { return PyDbDate(d.tduupdate()); })
		.def("tdindwg", [](const PyDbDatabase& d) { return PyDbDate(d.tdindwg()); })
		.def("tdusrtimer", [](const PyDbDatabase& d) { return PyDbDate(d.tdusrtimer()); })
		.def("resetTimes", &PyDbDatabase::resetTimes)
		.def("updateExt", [](PyDbDatabase& d, bool doBestFit) -> bool {
			return d.updateExt(doBestFit);
		}, py::arg("doBestFit") = false, "重算图形范围；成功返回 true")

		.def("purge", &PyDbDatabase::purge)
		.def("setFullSaveRequired", &PyDbDatabase::setFullSaveRequired)
		.def("saveAs", [](PyDbDatabase& self, const AcString& fileName) -> Acad::ErrorStatus {
			return self.saveAs(fileName, nullptr);
		}, py::arg("fileName"), "保存为 DWG 文件（返回 ErrorStatus，eOk=0）")
		.def("saveAs", [](PyDbDatabase& self, const AcString& fileName, bool bBakAndRename, int dwgVer) -> Acad::ErrorStatus {
			return self.saveAs(fileName, bBakAndRename, static_cast<AcDb::AcDbDwgVersion>(dwgVer), nullptr);
		}, py::arg("fileName"), py::arg("bBakAndRename"), py::arg("dwgVer"), "保存并指定版本（dwgVer 用整数，对应 AcDb::AcDbDwgVersion 枚举值）")
		.def("readDwgFile", [](PyDbDatabase& d, const AcString& fileName, int shmode, bool bAllowCPConversion, const AcString& wszPassword) -> Acad::ErrorStatus {
			const wchar_t* fp = fileName.kwszPtr();
			if (!fp || !std::filesystem::exists(fp)) {
				acutPrintf(L"[PyArx] readDwgFile: 文件不存在 %ls\n", fp ? fp : L"(null)");
				return Acad::eFileNotFound;
			}
			// DWG 文件头校验：合法 DWG 以 ASCII "AC10" 开头；损坏/非 DWG 文件直接拒绝，
			// 避免 acdb 读非法文件触发进程级 0xC0000005（Python 无法捕获）。
			{
				std::ifstream ifs(fp, std::ios::binary);
				unsigned char hdr[4] = { 0, 0, 0, 0 };
				if (!ifs.read(reinterpret_cast<char*>(hdr), 4)) {
					acutPrintf(L"[PyArx] readDwgFile: 文件不可读或过小 %ls\n", fp);
					return Acad::eFileNotFound;
				}
				if (hdr[0] != 'A' || hdr[1] != 'C' || hdr[2] != '1' || hdr[3] != '0') {
					acutPrintf(L"[PyArx] readDwgFile: 非法 DWG 文件头(非 AC10) %ls\n", fp);
					return Acad::eFileNotFound;
				}
			}
			// AcDbDatabase::OpenMode 合法值 1-4（kForReadAndReadShare=1 ... kTryForReadShare=4）；
			// 0 或越界值会导致 acdb 内部崩溃，clamp 到默认共享读模式 1。
			if (shmode < 1 || shmode > 4) shmode = 1;
			return d.readDwgFile(fileName, static_cast<AcDbDatabase::OpenMode>(shmode), bAllowCPConversion, wszPassword);
		}, py::arg("fileName"), py::arg("shmode") = 1, py::arg("bAllowCPConversion") = false, py::arg("wszPassword") = L"",
			"打开 DWG 文件；shmode 默认 1=AcDbDatabase::kForReadAndReadShare 共享读")
		.def("needsRecovery", &PyDbDatabase::needsRecovery)
		.def("isBeingDestroyed", &PyDbDatabase::isBeingDestroyed)
		// ---- 对象追加 / 非持久对象标记 ----
		.def("addAcDbObject", [](PyDbDatabase& d, PyDbObject& obj) -> uint64_t {
			auto id = d.addAcDbObject(obj);
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		}, py::arg("obj"), "向数据库追加一个已创建的对象，返回其 ObjectId(uint64)")
		.def("addObject", [](PyDbDatabase& d, PyDbObject& obj) -> uint64_t {
			auto id = d.addAcDbObject(obj);
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		}, py::arg("object"), "PyRx 别名：addObject，等价 addAcDbObject")
		// --- 便捷方法：添加入库 ---
		.def("addToModelspace", &PyDbDatabase::addToModelspace, py::arg("entity"), "添加实体到模型空间，返回新 ObjectId")
		.def("addToCurrentspace", &PyDbDatabase::addToCurrentspace, py::arg("entity"), "添加实体到当前空间，返回新 ObjectId")
		.def("addToBlock", &PyDbDatabase::addToBlock, py::arg("blockId"), py::arg("entity"), "添加实体到指定 Block，返回新 ObjectId")
		// --- 遍历对象 ---
		.def("objectIds", &PyDbDatabase::objectIds, py::arg("className") = "AcDbEntity", "遍历模型空间所有对象，返回 ObjectId 列表")
		// --- 系统变量统一访问 ---
		.def("getSysVar", &PyDbDatabase::getSysVar, py::arg("name"), "获取系统变量，如 db.getSysVar('OSMODE')")
		.def("setSysVar", &PyDbDatabase::setSysVar, py::arg("name"), py::arg("value"), "设置系统变量，如 db.setSysVar('OSMODE', 1)")
		// --- 文件读写 ---
		.def("dxfIn", [](PyDbDatabase& d, const std::string& filename) -> bool { return d.dxfIn(filename); }, py::arg("filename"), "从 DXF 文件导入")
		.def("dxfOut", [](PyDbDatabase& d, const std::string& filename, int precision) -> bool { return d.dxfOut(filename, precision); }, py::arg("filename"), py::arg("precision") = 16, "导出为 DXF 文件")
		.def("isObjectNonPersistent", [](PyDbDatabase& d, uint64_t objId) -> bool {
			AcDbObjectId id; id.setFromOldId(objId);
			return d.isObjectNonPersistent(id);
		}, py::arg("objId"), "对象是否为非持久对象")
		.def("markObjectNonPersistent", [](PyDbDatabase& d, uint64_t objId, bool value) -> bool {
			AcDbObjectId id; id.setFromOldId(objId);
			return PyWarnIfError(d.markObjectNonPersistent(id, value), L"Database.markObjectNonPersistent");
		}, py::arg("objId"), py::arg("value"), "标记/取消标记对象为非持久")
		// ---- 符号表（mode 0读/1写/2通知） ----
		.def("getBlockTable", [](PyDbDatabase& d, int mode) -> PyDbBlockTable {
			return PyDbBlockTable(d.getBlockTable(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		.def("blockTable", [](PyDbDatabase& d, int mode) -> PyDbBlockTable {
			return PyDbBlockTable(d.getBlockTable(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0, "PyRx 别名：blockTable")
		.def("getLayerTable", [](PyDbDatabase& d, int mode) -> PyDbLayerTable {
			return PyDbLayerTable(d.getLayerTable(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		.def("getTextStyleTable", [](PyDbDatabase& d, int mode) -> PyDbTextStyleTable {
			return PyDbTextStyleTable(d.getTextStyleTable(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		.def("getLinetypeTable", [](PyDbDatabase& d, int mode) -> PyDbLinetypeTable {
			return PyDbLinetypeTable(d.getLinetypeTable(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		.def("getViewTable", [](PyDbDatabase& d, int mode) -> PyDbViewTable {
			return PyDbViewTable(d.getViewTable(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		.def("getUCSTable", [](PyDbDatabase& d, int mode) -> PyDbUCSTable {
			return PyDbUCSTable(d.getUCSTable(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		.def("getViewportTable", [](PyDbDatabase& d, int mode) -> PyDbViewportTable {
			return PyDbViewportTable(d.getViewportTable(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		.def("getRegAppTable", [](PyDbDatabase& d, int mode) -> PyDbRegAppTable {
			return PyDbRegAppTable(d.getRegAppTable(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		// ---- RegApp 应用名注册（XData 写入前置） ----
		.def("registerApp", &PyDbDatabase::registerApp, py::arg("appName"), "注册 XData 应用名；成功返回 true")
		.def("isAppRegistered", &PyDbDatabase::isAppRegistered, py::arg("appName"), "应用名是否已注册")
		.def("getDimStyleTable", [](PyDbDatabase& d, int mode) -> PyDbDimStyleTable {
			return PyDbDimStyleTable(d.getDimStyleTable(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		// ---- 字典 ----
		.def("getGroupDictionary", [](PyDbDatabase& d, int mode) -> PyDbDictionary {
			return PyDbDictionary(d.getGroupDictionary(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		.def("getMLStyleDictionary", [](PyDbDatabase& d, int mode) -> PyDbDictionary {
			return PyDbDictionary(d.getMLStyleDictionary(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		.def("getLayoutDictionary", [](PyDbDatabase& d, int mode) -> PyDbDictionary {
			return PyDbDictionary(d.getLayoutDictionary(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		.def("getPlotStyleNameDictionary", [](PyDbDatabase& d, int mode) -> PyDbDictionary {
			return PyDbDictionary(static_cast<AcDbDictionary*>(d.getPlotStyleNameDictionary(static_cast<AcDb::OpenMode>(mode))));
		}, py::arg("mode") = 0)
		.def("getMaterialDictionary", [](PyDbDatabase& d, int mode) -> PyDbDictionary {
			return PyDbDictionary(d.getMaterialDictionary(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		.def("getVisualStyleDictionary", [](PyDbDatabase& d, int mode) -> PyDbDictionary {
			return PyDbDictionary(d.getVisualStyleDictionary(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		.def("getColorDictionary", [](PyDbDatabase& d, int mode) -> PyDbDictionary {
			return PyDbDictionary(d.getColorDictionary(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		.def("getScaleListDictionary", [](PyDbDatabase& d, int mode) -> PyDbDictionary {
			return PyDbDictionary(d.getScaleListDictionary(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		.def("getNamedObjectsDictionary", [](PyDbDatabase& d, int mode) -> PyDbDictionary {
			return PyDbDictionary(d.getNamedObjectsDictionary(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		.def("getPlotSettingsDictionary", [](PyDbDatabase& d, int mode) -> PyDbDictionary {
			return PyDbDictionary(d.getPlotSettingsDictionary(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		.def("getDataLinkDictionary", [](PyDbDatabase& d, int mode) -> PyDbDictionary {
			return PyDbDictionary(d.getDataLinkDictionary(static_cast<AcDb::OpenMode>(mode)));
		}, py::arg("mode") = 0)
		// ---- 区块管理器（无参版本返回 ObjectId；指针版本无包装类，未绑） ----
		.def("getSectionManager", [](const PyDbDatabase& d) -> uint64_t {
			auto id = d.getSectionManager();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		}, "区块管理器 ObjectId(uint64)")
		// ---- 数据链接更新 ----
		.def("updateDataLink", [](PyDbDatabase& d, int nDir, int nOption) -> bool {
			return PyWarnIfError(d.updateDataLink(static_cast<AcDb::UpdateDirection>(nDir),
				static_cast<AcDb::UpdateOption>(nOption)), L"Database.updateDataLink");
		}, py::arg("nDir"), py::arg("nOption"), "更新数据链接；成功返回 true")
		// ---- transaction manager (dbtrans) ----
		.def("transactionManager", [](const PyDbDatabase& d) {
			return PyDbTransactionManager(d.transactionManager());
		}, "获取数据库事务管理器 (AcDbTransactionManager)")
		// ---- 已实现未绑定补齐（2026-10-21）：save / getFilename / 管理器入口 ----
		.def("save", [](PyDbDatabase& d) -> bool {
			return PyWarnIfError(d.save(), L"AcDbDatabase::save");
		}, "保存当前图形文件（原地保存）；成功返回 true")
		.def("getFilename", &PyDbDatabase::getFilename,
			"返回当前图形文件名；失败返回空串")
		.def("getLayerStateManager", [](const PyDbDatabase& d) {
			return PyDbDbLayerStateManager(d.getLayerStateManager());
		}, "获取图层状态管理器 (PyDbDbLayerStateManager)")
		.def("objectContextManager", [](const PyDbDatabase& d) {
			return PyDbObjectContextManager(d.objectContextManager());
		}, "获取对象上下文管理器 (PyDbObjectContextManager)")
		// ---- 输入队列 ----
		.def("closeInput", [](PyDbDatabase& d) {
			return d.impObj()->closeInput(true) == Acad::eOk;
			}, "关闭挂起的输入队列")
		// ---- PyRx getter 对齐补齐（自动生成，纯读无副作用） ----
		.def("UCSTableId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->UCSTableId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("angdir", [](const PyDbDatabase& d) -> bool { return d.impObj()->angdir(); })
		.def("annoAllVisible", [](const PyDbDatabase& d) -> bool { return d.impObj()->annoAllVisible(); })
		.def("annotativeDwg", [](const PyDbDatabase& d) -> bool { return d.impObj()->annotativeDwg(); })
		.def("attdia", [](const PyDbDatabase& d) -> bool { return d.impObj()->attdia(); })
		.def("attreq", [](const PyDbDatabase& d) -> bool { return d.impObj()->attreq(); })
		.def("cDynDisplayMode", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->cDynDisplayMode()); })
		.def("cameraDisplay", [](const PyDbDatabase& d) -> bool { return d.impObj()->cameraDisplay(); })
		.def("cameraHeight", [](const PyDbDatabase& d) -> double { return d.impObj()->cameraHeight(); })
		.def("chamferc", [](const PyDbDatabase& d) -> double { return d.impObj()->chamferc(); })
		.def("chamferd", [](const PyDbDatabase& d) -> double { return d.impObj()->chamferd(); })
		.def("cmljust", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->cmljust()); })
		.def("cmlscale", [](const PyDbDatabase& d) -> double { return d.impObj()->cmlscale(); })
		.def("cmlstyleID", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->cmlstyleID(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("colorDictionaryId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->colorDictionaryId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("coords", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->coords()); })
		.def("dataLinkDictionaryId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->dataLinkDictionaryId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("delUsedObj", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->delUsedObj()); })
		.def("detailViewStyle", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->detailViewStyle(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("detailViewStyleDictionaryId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->detailViewStyleDictionaryId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("dimfit", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->dimfit()); })
		.def("dimunit", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->dimunit()); })
		.def("dispSilh", [](const PyDbDatabase& d) -> bool { return d.impObj()->dispSilh(); })
		.def("dragVisStyle", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->dragVisStyle(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("dragmode", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->dragmode()); })
		.def("dxeVal", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->dxeVal()); })
		.def("facetres", [](const PyDbDatabase& d) -> double { return d.impObj()->facetres(); })
		.def("geoCoordinateSystemId", [](const PyDbDatabase& d) -> AcString { return d.impObj()->geoCoordinateSystemId(); })
		.def("geoMarkerVisibility", [](const PyDbDatabase& d) -> bool { return d.impObj()->geoMarkerVisibility(); })
		.def("get3dDwfPrec", [](const PyDbDatabase& d) -> double { return d.impObj()->get3dDwfPrec(); })
		.def("getSymbolTableId", [](const PyDbDatabase& d) -> uint64_t {
			// 模板签名需显式指定表类型；PyRx 侧默认语义为块表（与 blockTableId 一致）
			auto id = d.impObj()->getSymbolTableId<AcDbBlockTable>();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		})
		.def("groupDictionaryId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->groupDictionaryId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("hpInherit", [](const PyDbDatabase& d) -> bool { return d.impObj()->hpInherit(); })
		.def("interfereObjVisStyle", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->interfereObjVisStyle(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("interfereVpVisStyle", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->interfereVpVisStyle(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("intersectColor", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->intersectColor()); })
		.def("isolines", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->isolines()); })
		.def("latitude", [](const PyDbDatabase& d) -> double { return d.impObj()->latitude(); })
		.def("layerNotify", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->layerNotify()); })
		.def("layoutDictionaryId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->layoutDictionaryId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("lensLength", [](const PyDbDatabase& d) -> double { return d.impObj()->lensLength(); })
		.def("lightsInBlocks", [](const PyDbDatabase& d) -> bool { return d.impObj()->lightsInBlocks(); })
		.def("lineWeightDisplay", [](const PyDbDatabase& d) -> bool { return d.impObj()->lineWeightDisplay(); })
		.def("loftAng1", [](const PyDbDatabase& d) -> double { return d.impObj()->loftAng1(); })
		.def("loftAng2", [](const PyDbDatabase& d) -> double { return d.impObj()->loftAng2(); })
		.def("loftMag1", [](const PyDbDatabase& d) -> double { return d.impObj()->loftMag1(); })
		.def("loftMag2", [](const PyDbDatabase& d) -> double { return d.impObj()->loftMag2(); })
		.def("loftParam", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->loftParam()); })
		.def("longitude", [](const PyDbDatabase& d) -> double { return d.impObj()->longitude(); })
		.def("mLStyleDictionaryId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->mLStyleDictionaryId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("materialDictionaryId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->materialDictionaryId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("maxactvp", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->maxactvp()); })
		.def("mleaderStyleDictionaryId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->mleaderStyleDictionaryId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("mleaderscale", [](const PyDbDatabase& d) -> double { return d.impObj()->mleaderscale(); })
		.def("mleaderstyle", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->mleaderstyle(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("msOleScale", [](const PyDbDatabase& d) -> double { return d.impObj()->msOleScale(); })
		.def("msltscale", [](const PyDbDatabase& d) -> bool { return d.impObj()->msltscale(); })
		.def("northDirection", [](const PyDbDatabase& d) -> double { return d.impObj()->northDirection(); })
		.def("obscuredColor", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->obscuredColor()); })
		.def("oleStartUp", [](const PyDbDatabase& d) -> bool { return d.impObj()->oleStartUp(); })
		.def("pdmode", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->pdmode()); })
		.def("pelevation", [](const PyDbDatabase& d) -> double { return d.impObj()->pelevation(); })
		.def("pickstyle", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->pickstyle()); })
		.def("plimcheck", [](const PyDbDatabase& d) -> bool { return d.impObj()->plimcheck(); })
		.def("plineEllipse", [](const PyDbDatabase& d) -> bool { return d.impObj()->plineEllipse(); })
		.def("plotSettingsDictionaryId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->plotSettingsDictionaryId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("plotStyleNameDictionaryId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->plotStyleNameDictionaryId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("previewType", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->previewType()); })
		.def("psolHeight", [](const PyDbDatabase& d) -> double { return d.impObj()->psolHeight(); })
		.def("psolWidth", [](const PyDbDatabase& d) -> double { return d.impObj()->psolWidth(); })
		.def("pucsBase", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->pucsBase(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("pucsname", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->pucsname(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("realWorldScale", [](const PyDbDatabase& d) -> bool { return d.impObj()->realWorldScale(); })
		.def("retainOriginalThumbnailBitmap", [](const PyDbDatabase& d) -> bool { return d.impObj()->retainOriginalThumbnailBitmap(); })
		.def("saveproxygraphics", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->saveproxygraphics()); })
		.def("scaleListDictionaryId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->scaleListDictionaryId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("sectionViewStyle", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->sectionViewStyle(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("sectionViewStyleDictionaryId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->sectionViewStyleDictionaryId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("shadedge", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->shadedge()); })
		.def("shadedif", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->shadedif()); })
		.def("shadowPlaneLocation", [](const PyDbDatabase& d) -> double { return d.impObj()->shadowPlaneLocation(); })
		.def("sketchinc", [](const PyDbDatabase& d) -> double { return d.impObj()->sketchinc(); })
		.def("skpoly", [](const PyDbDatabase& d) -> bool { return d.impObj()->skpoly(); })
		.def("splframe", [](const PyDbDatabase& d) -> bool { return d.impObj()->splframe(); })
		.def("splinesegs", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->splinesegs()); })
		.def("splinetype", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->splinetype()); })
		.def("stepSize", [](const PyDbDatabase& d) -> double { return d.impObj()->stepSize(); })
		.def("stepsPerSec", [](const PyDbDatabase& d) -> double { return d.impObj()->stepsPerSec(); })
		.def("surftab1", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->surftab1()); })
		.def("surftab2", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->surftab2()); })
		.def("surftype", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->surftype()); })
		.def("surfu", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->surfu()); })
		.def("surfv", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->surfv()); })
		.def("tableStyleDictionaryId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->tableStyleDictionaryId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("tablestyle", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->tablestyle(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("tracewid", [](const PyDbDatabase& d) -> double { return d.impObj()->tracewid(); })
		.def("treedepth", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->treedepth()); })
		.def("tstackalign", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->tstackalign()); })
		.def("tstacksize", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->tstacksize()); })
		.def("ucsBase", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->ucsBase(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("ucsname", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->ucsname(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("unitmode", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->unitmode()); })
		.def("useri1", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->useri1()); })
		.def("useri2", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->useri2()); })
		.def("useri3", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->useri3()); })
		.def("useri4", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->useri4()); })
		.def("useri5", [](const PyDbDatabase& d) -> int { return static_cast<int>(d.impObj()->useri5()); })
		.def("userr3", [](const PyDbDatabase& d) -> double { return d.impObj()->userr3(); })
		.def("userr4", [](const PyDbDatabase& d) -> double { return d.impObj()->userr4(); })
		.def("userr5", [](const PyDbDatabase& d) -> double { return d.impObj()->userr5(); })
		.def("usrtimer", [](const PyDbDatabase& d) -> bool { return d.impObj()->usrtimer(); })
		.def("visretain", [](const PyDbDatabase& d) -> bool { return d.impObj()->visretain(); })
		.def("visualStyleDictionaryId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->visualStyleDictionaryId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("worldview", [](const PyDbDatabase& d) -> bool { return d.impObj()->worldview(); })
		.def("xrefBlockId", [](const PyDbDatabase& d) -> uint64_t { auto id = d.impObj()->xrefBlockId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("xrefEditEnabled", [](const PyDbDatabase& d) -> bool { return d.impObj()->xrefEditEnabled(); })
		;

	// ------------------------------------------------------------------
	// 当前文档数据库 + 手动文档锁（不用 PyDbTrans 时使用）
	// ------------------------------------------------------------------
	m.def("curDb", []() {
		return PyDbDatabase(acdbHostApplicationServices()->workingDatabase(), false);
	}, "当前文档的 AcDbDatabase（不拥有）");

	m.def("lockDocument", []() -> int {
		AcApDocument* doc = acDocManager->curDocument();
		if (doc == nullptr)
			return static_cast<int>(Acad::eNoDocument);
		AcAp::DocLockMode mode = doc->myLockMode();
		if (mode == AcAp::kWrite)
			return static_cast<int>(Acad::eOk);
		return static_cast<int>(acDocManager->lockDocument(doc, AcAp::kWrite));
	}, "手动锁当前文档（kWrite）；返回 Acad::ErrorStatus 整数值");

	m.def("unlockDocument", []() -> int {
		AcApDocument* doc = acDocManager->curDocument();
		if (doc == nullptr)
			return static_cast<int>(Acad::eNoDocument);
		return static_cast<int>(acDocManager->unlockDocument(doc));
	}, "手动解锁当前文档");

	m.def("isDocumentLocked", []() -> bool {
		AcApDocument* doc = acDocManager->curDocument();
		return doc != nullptr && doc->myLockMode() != AcAp::kNone;
	}, "当前文档是否已锁");
}
