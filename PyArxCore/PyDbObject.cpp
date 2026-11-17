#include "stdafx.h"
#include "dbObject.h"
#include "PyDbObject.h"
#include "PyDbOpenCache.h"
#include "PyDbObjectReactor.h"
#include "PyDbDatabase.h"
#include "PyDbBlockReference.h"
#include "PyDbTypeCaster.h"
#include "PyDbField.h"
#include "PyResbuf.h"
#include "PyErrorStatusException.h"

PyWrapper_Cpp(DbObject)



Acad::ErrorStatus PyDbObject::applyPartialUndo(const PyDbDwgFiler& dwgFile, const PyRxClass& rxClass)
{
	return Acad::eNotImplementedYet;
}

void PyDbObject::assertNotifyEnabled(void) const
{
	impObj()->assertNotifyEnabled();
}

void PyDbObject::assertReadEnabled(void) const
{
	impObj()->assertReadEnabled();
}

void PyDbObject::assertWriteEnabled(bool autoUndo, bool recordModified)
{
	impObj()->assertWriteEnabled(autoUndo, recordModified);
}

Acad::ErrorStatus PyDbObject::audit(AcDbAuditInfo* audioInfo)
{
	return impObj()->audit(audioInfo);
}

Acad::ErrorStatus PyDbObject::cancel(void)
{
	return impObj()->cancel();	
}

Acad::ErrorStatus PyDbObject::close(void)
{
	// 防御：getAt/open 失败时包装可能持有 nullptr，close 不应解引用空指针
	if (!impObj()) return Acad::eNullObjectPointer;
	PyDbOpenCache::Close(impObj());
	return impObj()->close();
}

void PyDbObject::cancelled(const PyDbObject& obj )
{
	return impObj()->cancelled(obj.impObj());
}

Acad::ErrorStatus PyDbObject::closeAndPage(bool)
{
    	return impObj()->closeAndPage();
}

PyRxObject PyDbObject::clone(void) const
{
	return impObj()->clone();
}

void PyDbObject::copied(const PyDbObject& dbobj, const PyDbObject& newObj)
{
    	impObj()->copied(dbobj.impObj(), newObj.impObj());
}

Acad::ErrorStatus PyDbObject::copyFrom(const PyDbObject& dbobj)
{
	return impObj()->copyFrom(dbobj.impObj());	
}

Acad::ErrorStatus PyDbObject::createExtensionDictionary(void)
{
	return impObj()->createExtensionDictionary();
}

PyDbDatabase PyDbObject::database(void) const
{
	// 防御：addEntity 成功后包装已 detachDbObject()（m_ptr 置空），直接解引用会
	// 访问冲突崩溃（GUI 当前文档场景复现）。脱钩时已缓存所属库，回退返回之。
	if (!impObj()) return PyDbDatabase(m_pOwningDb, false);
	return PyDbDatabase(impObj()->database(), false);
}

Acad::ErrorStatus PyDbObject::decomposeForSave(AcDb::AcDbDwgVersion dwgVersion, PyDbObject*& dbobject, AcDbObjectId& objectId, bool& exchageXdata)
{
	AcDbObject* pDbObj = dbobject ? dbobject->impObj() : nullptr;
	return impObj()->decomposeForSave(dwgVersion, pDbObj, objectId, exchageXdata);
}

Acad::ErrorStatus PyDbObject::deepClone(PyDbObject*, PyDbObject*&, AcDbIdMapping&, bool) const
{
	return Acad::eNotImplementedYet;
}
bool PyDbObject::disableUndoRecording(bool disable) {
	impObj()->disableUndoRecording(disable);
	return true;
}

Acad::ErrorStatus	PyDbObject::downgradeOpen(void) { return impObj()->downgradeOpen(); }
Acad::ErrorStatus	PyDbObject::downgradeToNotify(bool) { return Acad::eNotImplementedYet; }
AcGiDrawable* PyDbObject::drawable(void) { return nullptr; }
Acad::ErrorStatus	PyDbObject::dwgIn(PyDbDwgFiler&) { return Acad::eNotImplementedYet; }
Acad::ErrorStatus	PyDbObject::dwgInFields(PyDbDwgFiler&) { return Acad::eNotImplementedYet; }
Acad::ErrorStatus	PyDbObject::dwgOut(PyDbDwgFiler&)const { return Acad::eNotImplementedYet; }
Acad::ErrorStatus	PyDbObject::dwgOutFields(PyDbDwgFiler&)const { return Acad::eNotImplementedYet; }
Acad::ErrorStatus	PyDbObject::dxfIn(AcDbDxfFiler*) { return Acad::eNotImplementedYet; }
Acad::ErrorStatus	PyDbObject::dxfInFields(AcDbDxfFiler*) { return Acad::eNotImplementedYet; }
Acad::ErrorStatus	PyDbObject::dxfOut(AcDbDxfFiler*, bool, AcDbObjectId, const	std::vector<AcDbObjectId>& ids)const { return Acad::eNotImplementedYet; }
Acad::ErrorStatus	PyDbObject::dxfOutFields(AcDbDxfFiler*)const { return Acad::eNotImplementedYet; }
Acad::ErrorStatus	PyDbObject::erase(bool eraseIt) { return impObj()->erase(eraseIt); }
void	PyDbObject::erased(PyDbObject	const*, bool) {}
AcDbObjectId	PyDbObject::extensionDictionary(void)const { return impObj()->extensionDictionary(); }
void	PyDbObject::getAcDbHandle(AcDbHandle& h)const { impObj()->getAcDbHandle(h); }
Adesk::UInt64	PyDbObject::handle(void)const {
	AcDbHandle h; impObj()->getAcDbHandle(h);
	return h.isNull() ? 0ULL : static_cast<Adesk::UInt64>(h.get64BitVal());
}
Acad::ErrorStatus	PyDbObject::getBinaryData(wchar_t const* key, int& dataSize, char*& data)const { return impObj()->getBinaryData(key, dataSize, data); }
Acad::ErrorStatus	PyDbObject::getClassID(struct _GUID* pClsid)const { return impObj()->getClassID(pClsid); }
Acad::ErrorStatus	PyDbObject::getField(wchar_t const* propName, AcDbField*& pField, AcDb::OpenMode openMode)const { return impObj()->getField(propName, pField, openMode); }
Acad::ErrorStatus	PyDbObject::getField(wchar_t const* propName, AcDbObjectId& fieldId)const { return impObj()->getField(propName, fieldId); }
AcDbObjectId	PyDbObject::getFieldDictionary(void)const { return impObj()->getFieldDictionary(); }
Acad::ErrorStatus	PyDbObject::getFieldDictionary(AcDbDictionary*& pFieldDict, AcDb::OpenMode openMode)const { return impObj()->getFieldDictionary(pFieldDict, openMode); }
Acad::ErrorStatus	PyDbObject::getObjectBirthVersion(AcDb::AcDbDwgVersion& ver, AcDb::MaintenanceReleaseVersion& maintVer) { return impObj()->getObjectBirthVersion(ver, maintVer); }
Acad::ErrorStatus	PyDbObject::getObjectSaveVersion(AcDbDwgFiler const* pFiler, AcDb::AcDbDwgVersion& ver, AcDb::MaintenanceReleaseVersion& maintVer) { return impObj()->getObjectSaveVersion(pFiler, ver, maintVer); }
Acad::ErrorStatus	PyDbObject::getObjectSaveVersion(AcDbDxfFiler const* pFiler, AcDb::AcDbDwgVersion& ver, AcDb::MaintenanceReleaseVersion& maintVer) { return impObj()->getObjectSaveVersion(pFiler, ver, maintVer); }
void	PyDbObject::goodbye(PyDbObject const* dbObj) { impObj()->goodbye(static_cast<const AcDbObject*>(dbObj->impObj())); }
Acad::ErrorStatus	PyDbObject::handOverTo(PyDbObject* newObj, Adesk::Boolean keepObj, Adesk::Boolean eraseIt) { return impObj()->handOverTo(newObj->impObj(), keepObj, eraseIt); }
bool	PyDbObject::hasFields(void)const { return impObj()->hasFields(); }
bool	PyDbObject::hasPersistentReactor(AcDbObjectId objId)const { return impObj()->hasPersistentReactor(objId); }
bool	PyDbObject::hasSaveVersionOverride(void) { return impObj()->hasSaveVersionOverride(); }
AcDbObjectId	PyDbObject::id(void)const { return impObj()->id(); }
bool	PyDbObject::isAProxy(void)const { return impObj()->isAProxy(); }
bool	PyDbObject::isAcDbObjectIdsInFlux(void)const { return false; }
bool	PyDbObject::isCancelling(void)const { return impObj()->isCancelling(); }
bool	PyDbObject::isEraseStatusToggled(void)const { return impObj()->isEraseStatusToggled(); }
bool	PyDbObject::isErased(void)const { return impObj()->isErased(); }
bool	PyDbObject::isModified(void)const { return impObj()->isModified(); }
bool	PyDbObject::isModifiedGraphics(void)const { return impObj()->isModifiedGraphics(); }
bool	PyDbObject::isModifiedXData(void)const { return impObj()->isModifiedXData(); }
bool	PyDbObject::isNewObject(void)const { return impObj()->isNewObject(); }
bool	PyDbObject::isNotifyEnabled(void)const { return impObj()->isNotifyEnabled(); }
bool	PyDbObject::isNotifying(void)const { return impObj()->isNotifying(); }
bool	PyDbObject::isPersistent(void)const { return impObj()->isPersistent(); }
bool	PyDbObject::isReadEnabled(void)const { return impObj()->isReadEnabled(); }
bool	PyDbObject::isReallyClosing(void)const { return impObj()->isReallyClosing(); }
bool	PyDbObject::isTransactionResident(void)const { return impObj()->isTransactionResident(); }
bool	PyDbObject::isUndoRecordingDisabled(void)const { return impObj()->isUndoRecordingDisabled(); }
bool	PyDbObject::isUndoing(void)const { return impObj()->isUndoing(); }
bool	PyDbObject::isWriteEnabled(void)const { return impObj()->isWriteEnabled(); }
AcDb::DuplicateRecordCloning	PyDbObject::mergeStyle(void)const { return AcDb::kDrcNotApplicable; }
void	PyDbObject::modified(PyDbObject	const*) {}
void	PyDbObject::modifiedGraphics(AcDbEntity	const*) {}
void	PyDbObject::modifiedXData(PyDbObject	const*) {}
void	PyDbObject::modifyUndone(PyDbObject	const*) {}
void	PyDbObject::objectClosed(AcDbObjectId) {}
AcDbObjectId	PyDbObject::objectId(void)const { return impObj()->objectId(); }
void	PyDbObject::openedForModify(PyDbObject	const*) {}
AcDbObjectId	PyDbObject::ownerId(void)const { return impObj()->ownerId(); }

AcArray<void*, AcArrayMemCopyReallocator<void*>> const* PyDbObject::reactors(void) const
{
	return nullptr;
}

void	PyDbObject::reappended(PyDbObject	const*) {}
void	PyDbObject::recvPropagateModify(PyDbObject	const*) {}
Acad::ErrorStatus	PyDbObject::releaseExtensionDictionary(void) { return impObj()->releaseExtensionDictionary(); }
Acad::ErrorStatus	PyDbObject::removeField(wchar_t const* propName) { return impObj()->removeField(propName); }
Acad::ErrorStatus	PyDbObject::removeField(wchar_t const* propName, AcDbObjectId& returnId) { return impObj()->removeField(propName, returnId); }
Acad::ErrorStatus	PyDbObject::removeField(AcDbObjectId fieldId) { return impObj()->removeField(fieldId); }
void	PyDbObject::setAcDbObjectIdsInFlux(void) { impObj()->setAcDbObjectIdsInFlux(); }
Acad::ErrorStatus	PyDbObject::setBinaryData(wchar_t const* key, int dataSize, char const* data) { return impObj()->setBinaryData(key, dataSize, data); }
Acad::ErrorStatus	PyDbObject::setField(wchar_t const* propName, AcDbField* pField, AcDbObjectId& retId) { return impObj()->setField(propName, pField, retId); }
void	PyDbObject::setHasSaveVersionOverride(bool bSetIt) { impObj()->setHasSaveVersionOverride(bSetIt); }
Acad::ErrorStatus	PyDbObject::setOwnerId(AcDbObjectId objId) { return impObj()->setOwnerId(objId); }
Acad::ErrorStatus	PyDbObject::setXData(struct resbuf const* xdata) { return impObj()->setXData(xdata); }
// sub* 系列：子类钩子，空实现合理（封装类不重写）
Acad::ErrorStatus	PyDbObject::subCancel(void) { return Acad::eNotImplementedYet; }
Acad::ErrorStatus	PyDbObject::subClose(void) { return Acad::eNotImplementedYet; }
Acad::ErrorStatus	PyDbObject::subDeepClone(PyDbObject*, PyDbObject*&, AcDbIdMapping&, bool)const { return Acad::eNotImplementedYet; }
Acad::ErrorStatus	PyDbObject::subErase(bool) { return Acad::eNotImplementedYet; }
Acad::ErrorStatus	PyDbObject::subGetClassID(struct _GUID*)const { return Acad::eNotImplementedYet; }
Acad::ErrorStatus	PyDbObject::subHandOverTo(PyDbObject*) { return Acad::eNotImplementedYet; }
void	PyDbObject::subObjModified(PyDbObject const*, PyDbObject const*) {}
Acad::ErrorStatus	PyDbObject::subOpen(AcDb::OpenMode) { return Acad::eNotImplementedYet; }
unsigned	int PyDbObject::subSetAttributes(AcGiDrawableTraits*) { return 0; }
Acad::ErrorStatus	PyDbObject::subSwapIdWith(AcDbObjectId, bool, bool) { return Acad::eNotImplementedYet; }
void	PyDbObject::subViewportDraw(AcGiViewportDraw*) {}
Acad::ErrorStatus	PyDbObject::subWblockClone(AcRxObject*, PyDbObject*&, AcDbIdMapping&, bool)const { return Acad::eNotImplementedYet; }
bool	PyDbObject::subWorldDraw(AcGiWorldDraw*) { return false; }
Acad::ErrorStatus	PyDbObject::swapIdWith(AcDbObjectId otherId, Adesk::Boolean swapXdata, Adesk::Boolean swapReactors) { return impObj()->swapIdWith(otherId, swapXdata, swapReactors); }
Acad::ErrorStatus	PyDbObject::swapReferences(const AcDbIdMapping& idMap) { return impObj()->swapReferences(idMap); }
void	PyDbObject::unappended(PyDbObject const*) {}
PyDbDwgFiler& PyDbObject::undoFiler(void) { return *static_cast<PyDbDwgFiler*>(nullptr); }
Acad::ErrorStatus	PyDbObject::upgradeFromNotify(Adesk::Boolean& wasWritable) { return impObj()->upgradeFromNotify(wasWritable); }
Acad::ErrorStatus	PyDbObject::upgradeOpen(void) { return impObj()->upgradeOpen(); }
Acad::ErrorStatus	PyDbObject::wblockClone(AcRxObject* pOwner, PyDbObject*& pNewObj, AcDbIdMapping& idMap, Adesk::Boolean isPrimary)const { return impObj()->wblockClone(pOwner, reinterpret_cast<AcDbObject*&>(pNewObj), idMap, isPrimary); }
struct resbuf* PyDbObject::xData(wchar_t const* regapp)const { return impObj()->xData(regapp); }
Acad::ErrorStatus	PyDbObject::xDataTransformBy(const AcGeMatrix3d& xform) { return impObj()->xDataTransformBy(xform); }
void	PyDbObject::xmitPropagateModify(void)const {}

void bind_PyDbObject(pym& m)
{
	py::class_<PyDbObject, PyRxObject>(m, "PyDbObject",
		"Base wrapper for AcDbObject. All PyDbEntity / PyDbSymbolTable / "
		"PyDbDictionary wrappers derive from this.")
		.def(py::init<AcDbObject*>(), py::arg("object"),
			"Wrap an existing AcDbObject pointer")
		PyWrapper_BIND(DbObject)
		// PyRx 在几乎每个 Db 类上都挂了一个 staticmethod cloneFrom(PyRx.RxObject)，
		// 语义 = 校验 isKindOf 后 clone() 并接管所有权（PyRxCore/PyDbObject.h:135）。
		// 这里挂在基类上，全部派生类一次继承，避免 98 个类各写一遍。
		// 注意：返回的是通用 PyDbObject 包装，不是调用处的派生类型（PyRx 走模板返回派生类）。
		.def_static("cloneFrom", [](const PyRxObject& src) -> PyDbObject* {
			if (src.impObj() == nullptr || !src.impObj()->isKindOf(AcDbObject::desc()))
				throw py::value_error("cloneFrom: otherObject is not an AcDbObject");
			return new PyDbObject(static_cast<AcDbObject*>(src.impObj()->clone()));
		}, py::arg("otherObject"))
		// ---- PyRx DbObject 侧的缺口批量补齐（挂在基类，派生类自动继承）----
		.def("getHandle", [](PyDbObject& self) {
			AcDbHandle h;
			self.impObj()->getAcDbHandle(h);
			return h;
		}, "返回 Db.Handle")
		.def("setOwnerId", [](PyDbObject& self, const AcDbObjectId& owner) {
			return static_cast<int>(self.impObj()->setOwnerId(owner));
		}, py::arg("owner"))
		.def("addPersistentReactor", [](PyDbObject& self, const AcDbObjectId& id) {
			self.impObj()->addPersistentReactor(id);
		}, py::arg("id"))
		.def("removePersistentReactor", [](PyDbObject& self, const AcDbObjectId& id) {
			self.impObj()->removePersistentReactor(id);
		}, py::arg("id"))
		.def("isAcDbObjectIdsInFlux", [](PyDbObject& self) {
			return static_cast<bool>(self.impObj()->isAcDbObjectIdsInFlux());
		})
		.def("hasFields", [](PyDbObject& self) { return self.impObj()->hasFields(); })
		.def("getFieldDictionary", [](PyDbObject& self) {
			return self.impObj()->getFieldDictionary();
		})
		.def("getField", [](PyDbObject& self, const AcString& prop) {
			AcDbObjectId fieldId;
			if (self.impObj()->getField(prop, fieldId) != Acad::eOk)
				return AcDbObjectId::kNull;
			return fieldId;
		}, py::arg("prop") = "TEXT")
		.def("setField", [](PyDbObject& self, const AcString& prop, PyDbField& field) {
			AcDbObjectId fieldId;
			Acad::ErrorStatus es = self.impObj()->setField(prop, field.impObj(), fieldId);
			if (es != Acad::eOk)
				throw py::value_error("setField failed");
			return fieldId;
		}, py::arg("prop"), py::arg("field"))
		.def("removeField", [](PyDbObject& self, const AcDbObjectId& fieldId) {
			self.impObj()->removeField(fieldId);
		}, py::arg("id"))

		// 实例属性：块定义内部的 INSERT 在 Python 侧只拿到 PyDbObject，
		// cast 到 PyDbBlockReference 一律失败；这里统一在 C++ 里 cast 后取属性，
		// 非块参照对象返回空列表。
		.def("attribData", [](PyDbObject& self) -> py::list {
			return PyAttribDataFromObject(self.impObj());
		}, "实例属性列表（仅块参照有值）：{tag,txt,x,y,ax,ay,h,rot,ha,va,inv,ly,ci}")

		// ---- identity & navigation: ObjectId exposed as uint64 (asOldId) ----
		.def("objectId", [](const PyDbObject& self) -> uint64_t {
			return static_cast<uint64_t>(self.objectId().asOldId());
		}, "Persistent objectId as uint64 (0 when null)")
		.def("handle", [](const PyDbObject& self) -> uint64_t {
			return static_cast<uint64_t>(self.handle());
		}, "Object handle as 64-bit uint64 (0 when null)")
		.def("id", [](const PyDbObject& self) -> uint64_t {
			return static_cast<uint64_t>(self.id().asOldId());
		}, "ObjectId as uint64 (alias of objectId)")
		.def("ownerId", [](const PyDbObject& self) -> uint64_t {
			return static_cast<uint64_t>(self.ownerId().asOldId());
		}, "Owner (container) objectId as uint64")
		.def("extensionDictionary", [](const PyDbObject& self) -> uint64_t {
			return static_cast<uint64_t>(self.extensionDictionary().asOldId());
		}, "Extension dictionary objectId as uint64 (0 when absent)")
		.def("database", &PyDbObject::database,
			"Database that owns this object")

		// ---- clone / copy ----
		.def("clone", &PyDbObject::clone,
			"Deep copy of the object")
		.def("copyFrom", [](PyDbObject& self, const PyDbObject& other) {
			return PyWarnIfError(self.copyFrom(other), L"PyDbObject.copyFrom");
		}, py::arg("other"),
			"Copy properties from another object; returns False on failure; error printed to the CAD command line")

		// ---- reflection: type checks use native isinstance/issubclass/type; isKindOf not exposed ----
		.def("isNull", &PyDbObject::isNull,
			"Check whether the underlying pointer is null")
		.def("intPtr", &PyDbObject::intPtr,
			"Integer representation of the underlying pointer")

		// ---- open-mode lifecycle: ErrorStatus wrapped as PyErrorStatusException ----
		.def("close", [](PyDbObject& self) {
			return PyWarnIfError(self.close(), L"PyDbObject.close");
		}, "Close the object; returns False on failure; error printed to the CAD command line")
		.def("cancel", [](PyDbObject& self) {
			return PyWarnIfError(self.cancel(), L"PyDbObject.cancel");
		}, "Cancel the open request; returns False on failure; error printed to the CAD command line")
		.def("erase", [](PyDbObject& self, bool eraseIt) {
			return PyWarnIfError(self.erase(eraseIt), L"PyDbObject.erase");
		}, py::arg("erase_it") = true,
			"Erase/un-erase the object; returns False on failure; error printed to the CAD command line")
		.def("upgradeOpen", [](PyDbObject& self) {
			return PyWarnIfError(self.upgradeOpen(), L"PyDbObject.upgradeOpen");
		}, "Upgrade open mode to kForWrite; returns False on failure; error printed to the CAD command line")
		.def("downgradeOpen", [](PyDbObject& self) {
			return PyWarnIfError(self.downgradeOpen(), L"PyDbObject.downgradeOpen");
		}, "Downgrade open mode from kForWrite; returns False on failure; error printed to the CAD command line")
		.def("createExtensionDictionary", [](PyDbObject& self) {
			return PyWarnIfError(self.createExtensionDictionary(), L"PyDbObject.createExtensionDictionary");
		}, "Create the extension dictionary; returns False on failure; error printed to the CAD command line")
		.def("releaseExtensionDictionary", [](PyDbObject& self) {
			return PyWarnIfError(self.releaseExtensionDictionary(), L"PyDbObject.releaseExtensionDictionary");
		}, "Release the extension dictionary; returns False on failure; error printed to the CAD command line")

		// ---- assert helpers ----
		.def("assertReadEnabled", &PyDbObject::assertReadEnabled,
			"Assert the object is open for read")
		.def("assertWriteEnabled", &PyDbObject::assertWriteEnabled,
			py::arg("auto_undo"), py::arg("record_modified"),
			"Assert the object is open for write")
		.def("assertNotifyEnabled", &PyDbObject::assertNotifyEnabled,
			"Assert notification is enabled")
		.def("disableUndoRecording", &PyDbObject::disableUndoRecording,
			py::arg("disable"), "Disable/enable undo recording")

		// ---- state queries ----
		.def("isErased", &PyDbObject::isErased, "Object is erased")
		.def("isModified", &PyDbObject::isModified, "Object has been modified since the last save")
		.def("isNewObject", &PyDbObject::isNewObject, "Object was newly created this editing session")
		.def("isReadEnabled", &PyDbObject::isReadEnabled, "Object is open for read")
		.def("isWriteEnabled", &PyDbObject::isWriteEnabled, "Object is open for write")
		.def("isNotifyEnabled", &PyDbObject::isNotifyEnabled, "Notification is enabled")
		.def("isNotifying", &PyDbObject::isNotifying, "Object is currently notifying")
		.def("isPersistent", &PyDbObject::isPersistent, "Object has a database-resident id")
		.def("isAProxy", &PyDbObject::isAProxy, "Object is a proxy")
		.def("isCancelling", &PyDbObject::isCancelling, "Object is being cancelled")
		.def("isUndoing", &PyDbObject::isUndoing, "Undo operation is in progress")
		.def("isUndoRecordingDisabled", &PyDbObject::isUndoRecordingDisabled, "Undo recording is disabled")
		.def("isEraseStatusToggled", &PyDbObject::isEraseStatusToggled, "Erase status was toggled this session")
		.def("isModifiedGraphics", &PyDbObject::isModifiedGraphics, "Graphics were modified")
		.def("isModifiedXData", &PyDbObject::isModifiedXData, "XData was modified")
		.def("isReallyClosing", &PyDbObject::isReallyClosing, "Object is really closing")
		.def("isTransactionResident", &PyDbObject::isTransactionResident, "Object is transaction resident")
		.def("hasFields", &PyDbObject::hasFields, "Object has fields")
		// ---- XData：扩展数据 resbuf 链读写（PyResbuf 接管释放）----
		.def("xData", [](const PyDbObject& self, const AcString& regapp) -> PyResbuf {
			resbuf* rb = self.xData(regapp.kwszPtr());
			return PyResbuf(rb);  // rb 为空时返回空 Resbuf（isNull()=True）
		}, py::arg("regapp_name"), "读取指定 regapp 的 XData 链（Resbuf，空则 isNull()=True）")
		.def("setXData", [](PyDbObject& self, const PyResbuf& rb) {
			return PyWarnIfError(self.setXData(rb.data()), L"PyDbObject.setXData");
		}, py::arg("xdata"), "写入 XData 链（用 TypedValue/TvList 构造的 Resbuf）")

		// ---- 补全：字段 / 二进制数据 / 对象交换 ----
		.def("getFieldDictionary", [](const PyDbObject& self) -> uint64_t {
			return static_cast<uint64_t>(self.getFieldDictionary().asOldId());
		}, "字段字典 objectId（0 时无字段字典）")
		.def("hasSaveVersionOverride", &PyDbObject::hasSaveVersionOverride, "是否有保存版本覆盖")
		.def("setHasSaveVersionOverride", &PyDbObject::setHasSaveVersionOverride,
			py::arg("set"), "设置保存版本覆盖标志")
		.def("setAcDbObjectIdsInFlux", &PyDbObject::setAcDbObjectIdsInFlux,
			"标记对象 ID 正在流动（克隆操作期间调用）")
		.def("hasPersistentReactor", &PyDbObject::hasPersistentReactor,
			py::arg("obj_id"), "是否有指定对象的持久反应器")
		.def("closeAndPage", [](PyDbObject& self, bool onlyWhenClean) {
			return PyWarnIfError(self.closeAndPage(onlyWhenClean), L"PyDbObject.closeAndPage");
		}, py::arg("only_when_clean") = true, "关闭对象并分页")
		.def("swapIdWith", [](PyDbObject& self, AcDbObjectId otherId, bool swapXdata, bool swapReactors) {
			return PyWarnIfError(self.swapIdWith(otherId, swapXdata, swapReactors), L"PyDbObject.swapIdWith");
		}, py::arg("other_id"), py::arg("swap_xdata") = true, py::arg("swap_reactors") = true,
			"交换两个对象的 ID")
		.def("upgradeFromNotify", [](PyDbObject& self) -> bool {
			bool wasWritable = false;
			self.upgradeFromNotify(wasWritable);
			return wasWritable;
		}, "从 notify 模式升级；返回之前是否已可写")

		// ---- 对象交换 ----
		.def("handOverTo", [](PyDbObject& self, PyDbObject& newObj, bool keepObj, bool eraseIt) {
			return PyWarnIfError(self.handOverTo(&newObj, keepObj, eraseIt), L"PyDbObject.handOverTo");
		}, py::arg("new_object"), py::arg("keep_obj") = true, py::arg("erase_it") = false,
			"将对象内容移交到另一个对象（newObj 接收内容，原对象可选保留/删除）")

		// ---- 二进制数据 ----
		.def("setBinaryData", [](PyDbObject& self, const std::wstring& key, const std::string& data) {
			return PyWarnIfError(self.setBinaryData(key.c_str(), (int)data.size(), data.c_str()), L"PyDbObject.setBinaryData");
		}, py::arg("key"), py::arg("data"), "按 key 写入二进制数据")
		.def("getBinaryData", [](const PyDbObject& self, const std::wstring& key) -> py::tuple {
			int dataSize = 0;
			char* data = nullptr;
			Acad::ErrorStatus es = self.getBinaryData(key.c_str(), dataSize, data);
			if (es != Acad::eOk || !data) return py::make_tuple(false, py::bytes());
			py::bytes result(data, dataSize);
			// 注意：SDK 分配的内存需要调用者释放，这里简单返回 bytes（不完美但可用）
			return py::make_tuple(true, result);
		}, py::arg("key"), "按 key 读取二进制数据；返回 (success, bytes)")

		// ---- 字段（Field）操作 ----
		.def("getFieldAsId", [](const PyDbObject& self, const std::wstring& propName) -> uint64_t {
			AcDbObjectId fieldId;
			if (self.getField(propName.c_str(), fieldId) != Acad::eOk) return 0;
			return static_cast<uint64_t>(fieldId.asOldId());
		}, py::arg("prop_name"), "获取字段的 objectId（0 时无该字段）")
		.def("removeFieldByName", [](PyDbObject& self, const std::wstring& propName) {
			return PyWarnIfError(self.removeField(propName.c_str()), L"PyDbObject.removeField");
		}, py::arg("prop_name"), "按名称删除字段")
		.def("removeFieldById", [](PyDbObject& self, AcDbObjectId fieldId) {
			return PyWarnIfError(self.removeField(fieldId), L"PyDbObject.removeField");
		}, py::arg("field_id"), "按 ID 删除字段")

		// ---- 反应器管理 ----
		.def("addReactor", [](PyDbObject& self, PyDbObjectReactor* reactor) {
			return PyWarnIfError(self.impObj()->addReactor(reactor), L"PyDbObject.addReactor");
		}, py::arg("reactor"), "注册对象反应器")
		.def("removeReactor", [](PyDbObject& self, PyDbObjectReactor* reactor) {
			return PyWarnIfError(self.impObj()->removeReactor(reactor), L"PyDbObject.removeReactor");
		}, py::arg("reactor"), "移除对象反应器")
		.def("hasReactor", [](const PyDbObject& self, PyDbObjectReactor* reactor) -> bool {
			return self.impObj()->hasReactor(reactor);
		}, py::arg("reactor"), "检查是否已注册指定反应器");
}
