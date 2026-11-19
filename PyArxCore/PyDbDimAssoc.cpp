#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDimAssoc.h"

PyWrapper_Cpp(DbDimAssoc);

bool PyDbDimAssoc::addToDimensionReactor(bool isAdd) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->addToDimensionReactor(isAdd), L"AcDbDimAssoc::addToDimensionReactor");
}
bool PyDbDimAssoc::addToPointRefReactor() {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->addToPointRefReactor(), L"AcDbDimAssoc::addToPointRefReactor");
}
int PyDbDimAssoc::assocFlag() { if (!impObj()) return 0; return impObj()->assocFlag(); }
bool PyDbDimAssoc::assocFlag(int ptType) const { if (!impObj()) return false; return impObj()->assocFlag(ptType); }
uint64_t PyDbDimAssoc::dimObjId() const {
	if (!impObj()) return 0ULL;
	AcDbObjectId id = impObj()->dimObjId();
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}
AcDbObjectIdArray PyDbDimAssoc::getDimAssocGeomIds() const {
	if (!impObj()) return AcDbObjectIdArray();
	AcDbObjectIdArray ids;
	PyWarnIfError(impObj()->getDimAssocGeomIds(ids), L"AcDbDimAssoc::getDimAssocGeomIds");
	return ids;
}
bool PyDbDimAssoc::hasOopsWatcher() const { if (!impObj()) return false; return impObj()->hasOopsWatcher(); }
bool PyDbDimAssoc::isAllGeomErased() const { if (!impObj()) return false; return impObj()->isAllGeomErased(); }
bool PyDbDimAssoc::isTransSpatial() const { if (!impObj()) return false; return impObj()->isTransSpatial(); }

std::tuple<bool, uint64_t> PyDbDimAssoc::post(uint64_t dimId, bool isActive) {
	if (!impObj()) return {};
	AcDbObjectId dim; dim.setFromOldId(static_cast<Adesk::UIntPtr>(dimId));
	AcDbObjectId outId;
	Acad::ErrorStatus es = impObj()->post(dim, outId, isActive);
	if (!PyWarnIfError(es, L"AcDbDimAssoc::post")) return std::make_tuple(false, 0ULL);
	return std::make_tuple(true, outId.isNull() ? 0ULL : static_cast<uint64_t>(outId.asOldId()));
}

bool PyDbDimAssoc::removeAssociativity(bool force) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->removeAssociativity(force), L"AcDbDimAssoc::removeAssociativity");
}
void PyDbDimAssoc::removeOopsWatcher() { if (!impObj()) return; impObj()->removeOopsWatcher(); }
bool PyDbDimAssoc::removePointRef(int ptType) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->removePointRef(ptType), L"AcDbDimAssoc::removePointRef");
}
bool PyDbDimAssoc::restoreAssocFromOopsWatcher() {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->restoreAssocFromOopsWatcher(), L"AcDbDimAssoc::restoreAssocFromOopsWatcher");
}
int PyDbDimAssoc::rotatedDimType() const { if (!impObj()) return 0; return (int)impObj()->rotatedDimType(); }
void PyDbDimAssoc::setAssocFlag(int assocFlg) { if (!impObj()) return; impObj()->setAssocFlag(assocFlg); }
bool PyDbDimAssoc::setAssocFlag(int ptType, bool value) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setAssocFlag(ptType, value), L"AcDbDimAssoc::setAssocFlag");
}
bool PyDbDimAssoc::setDimObjId(uint64_t dimId) {
	if (!impObj()) return false;
	AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(dimId));
	return PyWarnIfError(impObj()->setDimObjId(id), L"AcDbDimAssoc::setDimObjId");
}
bool PyDbDimAssoc::setRotatedDimType(int dimType) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setRotatedDimType(static_cast<AcDbDimAssoc::RotatedDimType>(dimType)), L"AcDbDimAssoc::setRotatedDimType");
}
bool PyDbDimAssoc::setTransSpatial(bool value) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setTransSpatial(value), L"AcDbDimAssoc::setTransSpatial");
}
bool PyDbDimAssoc::startCmdWatcher() {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->startCmdWatcher(), L"AcDbDimAssoc::startCmdWatcher");
}
bool PyDbDimAssoc::startOopsWatcher(bool bAddAll) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->startOopsWatcher(bAddAll), L"AcDbDimAssoc::startOopsWatcher");
}
bool PyDbDimAssoc::updateAssociativity(const AcDbObjectIdArray& ids) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->updateAssociativity(ids), L"AcDbDimAssoc::updateAssociativity");
}
void PyDbDimAssoc::updateDueToMirror(bool wasInMirror) { if (!impObj()) return; impObj()->updateDueToMirror(wasInMirror); }
bool PyDbDimAssoc::updateFillet(const AcDbObjectIdArray& ids) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->updateFillet(ids), L"AcDbDimAssoc::updateFillet");
}
bool PyDbDimAssoc::updateXrefSubentPath() {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->updateXrefSubentPath(), L"AcDbDimAssoc::updateXrefSubentPath");
}

void bind_PyDbDimAssoc(pym& m) {
	py::class_<PyDbDimAssoc, PyDbObject>(m, "PyDbDimAssoc")
		.def(py::init<>())
		.def(py::init<AcDbDimAssoc*>(), py::arg("p"))
		PyWrapper_BIND(DbDimAssoc)
		.def("addToDimensionReactor", &PyDbDimAssoc::addToDimensionReactor, py::arg("isAdd") = true)
		.def("addToPointRefReactor", &PyDbDimAssoc::addToPointRefReactor)
		.def("assocFlag", [](PyDbDimAssoc& d) { return d.assocFlag(); })
		.def("assocFlag", [](PyDbDimAssoc& d, int ptType) { return d.assocFlag(ptType); }, py::arg("ptType"))
		.def("dimObjId", &PyDbDimAssoc::dimObjId)
		.def("getDimAssocGeomIds", &PyDbDimAssoc::getDimAssocGeomIds)
		.def("hasOopsWatcher", &PyDbDimAssoc::hasOopsWatcher)
		.def("isAllGeomErased", &PyDbDimAssoc::isAllGeomErased)
		.def("isTransSpatial", &PyDbDimAssoc::isTransSpatial)
		.def("post", &PyDbDimAssoc::post, py::arg("dimId"), py::arg("isActive") = true, "返回 (ok, outDimAssocId)")
		.def("removeAssociativity", &PyDbDimAssoc::removeAssociativity, py::arg("force") = true)
		.def("removeOopsWatcher", &PyDbDimAssoc::removeOopsWatcher)
		.def("removePointRef", &PyDbDimAssoc::removePointRef, py::arg("ptType"))
		.def("restoreAssocFromOopsWatcher", &PyDbDimAssoc::restoreAssocFromOopsWatcher)
		.def("rotatedDimType", &PyDbDimAssoc::rotatedDimType)
		.def("setAssocFlag", [](PyDbDimAssoc& d, int f) { d.setAssocFlag(f); }, py::arg("assocFlg"))
		.def("setAssocFlag", static_cast<bool (PyDbDimAssoc::*)(int, bool)>(&PyDbDimAssoc::setAssocFlag), py::arg("ptType"), py::arg("value"))
		.def("setDimObjId", &PyDbDimAssoc::setDimObjId, py::arg("dimId"))
		.def("setRotatedDimType", &PyDbDimAssoc::setRotatedDimType, py::arg("dimType"))
		.def("setTransSpatial", &PyDbDimAssoc::setTransSpatial, py::arg("value"))
		.def("startCmdWatcher", &PyDbDimAssoc::startCmdWatcher)
		.def("startOopsWatcher", &PyDbDimAssoc::startOopsWatcher, py::arg("bAddAll") = true)
		.def("updateAssociativity", &PyDbDimAssoc::updateAssociativity, py::arg("ids"))
		.def("updateDueToMirror", &PyDbDimAssoc::updateDueToMirror, py::arg("wasInMirror"))
		.def("updateFillet", &PyDbDimAssoc::updateFillet, py::arg("ids"))
		.def("updateXrefSubentPath", &PyDbDimAssoc::updateXrefSubentPath)
		// 未绑：setPointRef（AcDbPointRef 未封装，P2 依赖）；
		//       setPointRefReactor 在 SDK 头中为 protected 成员（dbdimassoc.h:126），无公开等价入口，跳过；
		//       updateDimension(11 个重载，参数为各类 Dimension/Leader/MLeader 指针，工程未全部封装)；
		//       updateSubentPath(AcDbIdMapping& 未封装)；dwg/dxf in/outFields、modifiedGraphics、swapReferences 为覆写。
		;
}
