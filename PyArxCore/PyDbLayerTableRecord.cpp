#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLayerTableRecord.h"

PyWrapper_Cpp(DbLayerTableRecord);

bool PyDbLayerTableRecord::isFrozen() const { return impObj()->isFrozen(); }
bool  PyDbLayerTableRecord::setIsFrozen(bool frozen) { return PyWarnIfError(impObj()->setIsFrozen(frozen), L"AcDbLayerTableRecord::setIsFrozen"); }
bool PyDbLayerTableRecord::isOff() const { return impObj()->isOff(); }
void PyDbLayerTableRecord::setIsOff(bool off) { impObj()->setIsOff(off); }
bool PyDbLayerTableRecord::VPDFLT() const { return impObj()->VPDFLT(); }
void PyDbLayerTableRecord::setVPDFLT(bool frozen) { impObj()->setVPDFLT(frozen); }
bool PyDbLayerTableRecord::isLocked() const { return impObj()->isLocked(); }
void PyDbLayerTableRecord::setIsLocked(bool locked) { impObj()->setIsLocked(locked); }
uint64_t PyDbLayerTableRecord::linetypeObjectId() const { return static_cast<uint64_t>(impObj()->linetypeObjectId().asOldId()); }
bool  PyDbLayerTableRecord::setLinetypeObjectId(uint64_t id) {
	AcDbObjectId oid;
	oid.setFromOldId(static_cast<Adesk::IntDbId>(id));
	return PyWarnIfError(impObj()->setLinetypeObjectId(oid), L"AcDbLayerTableRecord::setLinetypeObjectId");
}
uint64_t PyDbLayerTableRecord::materialId() const { return static_cast<uint64_t>(impObj()->materialId().asOldId()); }
bool  PyDbLayerTableRecord::setMaterialId(uint64_t id) {
	AcDbObjectId oid;
	oid.setFromOldId(static_cast<Adesk::IntDbId>(id));
	return PyWarnIfError(impObj()->setMaterialId(oid), L"AcDbLayerTableRecord::setMaterialId");
}
int PyDbLayerTableRecord::lineWeight() const { return static_cast<int>(impObj()->lineWeight()); }
bool  PyDbLayerTableRecord::setLineWeight(int lw) { return PyWarnIfError(impObj()->setLineWeight(static_cast<AcDb::LineWeight>(lw)), L"AcDbLayerTableRecord::setLineWeight"); }
bool PyDbLayerTableRecord::isPlottable() const { return impObj()->isPlottable(); }
bool  PyDbLayerTableRecord::setIsPlottable(bool plot) { return PyWarnIfError(impObj()->setIsPlottable(plot), L"AcDbLayerTableRecord::setIsPlottable"); }
AcString PyDbLayerTableRecord::plotStyleName() const { AcString s; if(!PyWarnIfError(impObj()->plotStyleName(s), L"AcDbLayerTableRecord::plotStyleName")) return AcString(); return s; }
uint64_t PyDbLayerTableRecord::plotStyleNameId() const { return static_cast<uint64_t>(impObj()->plotStyleNameId().asOldId()); }
bool  PyDbLayerTableRecord::setPlotStyleName(const AcString& newName) { return PyWarnIfError(impObj()->setPlotStyleName(newName.kwszPtr()), L"AcDbLayerTableRecord::setPlotStyleName"); }
bool  PyDbLayerTableRecord::setPlotStyleNameById(uint64_t newId) {
	AcDbObjectId oid;
	oid.setFromOldId(static_cast<Adesk::IntDbId>(newId));
	return PyWarnIfError(impObj()->setPlotStyleName(oid), L"AcDbLayerTableRecord::setPlotStyleNameById");
}
bool PyDbLayerTableRecord::isInUse() const { return impObj()->isInUse(); }
AcString PyDbLayerTableRecord::description() const { AcString s; if(!PyWarnIfError(impObj()->description(s), L"AcDbLayerTableRecord::description")) return AcString(); return s; }
bool  PyDbLayerTableRecord::setDescription(const AcString& description) { return PyWarnIfError(impObj()->setDescription(description.kwszPtr()), L"AcDbLayerTableRecord::setDescription"); }
bool PyDbLayerTableRecord::isHidden() const { return impObj()->isHidden(); }
bool  PyDbLayerTableRecord::setIsHidden(bool on) { return PyWarnIfError(impObj()->setIsHidden(on), L"AcDbLayerTableRecord::setIsHidden"); }
bool PyDbLayerTableRecord::isHidden(uint64_t objectId) {
	AcDbObjectId oid;
	oid.setFromOldId(static_cast<Adesk::IntDbId>(objectId));
	return AcDbLayerTableRecord::isHidden(oid);
}
bool  PyDbLayerTableRecord::removeColorOverride(uint64_t viewportId) {
	AcDbObjectId oid;
	oid.setFromOldId(static_cast<Adesk::IntDbId>(viewportId));
	return PyWarnIfError(impObj()->removeColorOverride(oid), L"AcDbLayerTableRecord::removeColorOverride");
}
bool  PyDbLayerTableRecord::removeLinetypeOverride(uint64_t viewportId) {
	AcDbObjectId oid;
	oid.setFromOldId(static_cast<Adesk::IntDbId>(viewportId));
	return PyWarnIfError(impObj()->removeLinetypeOverride(oid), L"AcDbLayerTableRecord::removeLinetypeOverride");
}
bool  PyDbLayerTableRecord::removeLineWeightOverride(uint64_t viewportId) {
	AcDbObjectId oid;
	oid.setFromOldId(static_cast<Adesk::IntDbId>(viewportId));
	return PyWarnIfError(impObj()->removeLineWeightOverride(oid), L"AcDbLayerTableRecord::removeLineWeightOverride");
}
bool  PyDbLayerTableRecord::removePlotStyleOverride(uint64_t viewportId) {
	AcDbObjectId oid;
	oid.setFromOldId(static_cast<Adesk::IntDbId>(viewportId));
	return PyWarnIfError(impObj()->removePlotStyleOverride(oid), L"AcDbLayerTableRecord::removePlotStyleOverride");
}
bool  PyDbLayerTableRecord::removeTransparencyOverride(uint64_t viewportId) {
	AcDbObjectId oid;
	oid.setFromOldId(static_cast<Adesk::IntDbId>(viewportId));
	return PyWarnIfError(impObj()->removeTransparencyOverride(oid), L"AcDbLayerTableRecord::removeTransparencyOverride");
}
bool  PyDbLayerTableRecord::removeViewportOverrides(uint64_t viewportId) {
	AcDbObjectId oid;
	oid.setFromOldId(static_cast<Adesk::IntDbId>(viewportId));
	return PyWarnIfError(impObj()->removeViewportOverrides(oid), L"AcDbLayerTableRecord::removeViewportOverrides");
}
bool  PyDbLayerTableRecord::removeAllOverrides() { return PyWarnIfError(impObj()->removeAllOverrides(), L"AcDbLayerTableRecord::removeAllOverrides"); }
bool PyDbLayerTableRecord::hasOverrides(uint64_t viewportId) const {
	AcDbObjectId oid;
	oid.setFromOldId(static_cast<Adesk::IntDbId>(viewportId));
	return impObj()->hasOverrides(oid);
}
bool PyDbLayerTableRecord::hasAnyOverrides() const { return impObj()->hasAnyOverrides(); }
bool PyDbLayerTableRecord::isReconciled() const { return impObj()->isReconciled(); }
bool  PyDbLayerTableRecord::setIsReconciled(bool bReconcile) { return PyWarnIfError(impObj()->setIsReconciled(bReconcile), L"AcDbLayerTableRecord::setIsReconciled"); }
bool PyDbLayerTableRecord::isReconciled(uint64_t objectId) {
	AcDbObjectId oid;
	oid.setFromOldId(static_cast<Adesk::IntDbId>(objectId));
	return AcDbLayerTableRecord::isReconciled(oid);
}

std::tuple<int, int, int, int> PyDbLayerTableRecord::color() const {
	AcCmColor c = impObj()->color();
	int ci = static_cast<int>(c.colorIndex());
	int r = static_cast<int>(c.red());
	int g = static_cast<int>(c.green());
	int b = static_cast<int>(c.blue());
	return std::make_tuple(ci, r, g, b);
}

void PyDbLayerTableRecord::setColor(int r, int g, int b) {
	AcCmColor c;
	c.setRGB(static_cast<Adesk::UInt8>(r), static_cast<Adesk::UInt8>(g), static_cast<Adesk::UInt8>(b));
	impObj()->setColor(c);
}

std::tuple<int, int, int, int> PyDbLayerTableRecord::entityColor() const {
	AcCmEntityColor ec = impObj()->entityColor();
	int ci = static_cast<int>(ec.colorIndex());
	int r = static_cast<int>(ec.red());
	int g = static_cast<int>(ec.green());
	int b = static_cast<int>(ec.blue());
	return std::make_tuple(ci, r, g, b);
}

int PyDbLayerTableRecord::transparency() const {
	AcCmTransparency t = impObj()->transparency();
	if (t.isByLayer()) return 0;
	if (t.isByBlock()) return 1;
	return static_cast<int>(t.alpha());
}

bool PyDbLayerTableRecord::setTransparency(int trans) {
	AcCmTransparency t;
	if (trans == 0) { t.setMethod(AcCmTransparency::kByLayer); }
	else if (trans == 1) { t.setMethod(AcCmTransparency::kByBlock); }
	else { t.setAlpha(static_cast<Adesk::UInt8>(trans)); }
	return PyWarnIfError(impObj()->setTransparency(t), L"AcDbLayerTableRecord::setTransparency");
}

void bind_PyDbLayerTableRecord(pym& m) {
	py::class_<PyDbLayerTableRecord, PyDbSymbolTableRecord>(m, "PyDbLayerTableRecord")
		.def(py::init<AcDbLayerTableRecord*>(), py::arg("p"))
		PyWrapper_BIND(DbLayerTableRecord)
		.def("isFrozen", &PyDbLayerTableRecord::isFrozen)
		.def("setIsFrozen", &PyDbLayerTableRecord::setIsFrozen, py::arg("frozen"))
		.def("isOff", &PyDbLayerTableRecord::isOff)
		.def("setIsOff", &PyDbLayerTableRecord::setIsOff, py::arg("off"))
		.def("vPDFLT", &PyDbLayerTableRecord::VPDFLT)
		.def("setVPDFLT", &PyDbLayerTableRecord::setVPDFLT, py::arg("frozen"))
		.def("isLocked", &PyDbLayerTableRecord::isLocked)
		.def("setIsLocked", &PyDbLayerTableRecord::setIsLocked, py::arg("locked"))
		.def("linetypeObjectId", &PyDbLayerTableRecord::linetypeObjectId)
		.def("setLinetypeObjectId", &PyDbLayerTableRecord::setLinetypeObjectId, py::arg("id"))
		.def("materialId", &PyDbLayerTableRecord::materialId)
		.def("setMaterialId", &PyDbLayerTableRecord::setMaterialId, py::arg("id"))
		.def("lineWeight", &PyDbLayerTableRecord::lineWeight)
		.def("setLineWeight", &PyDbLayerTableRecord::setLineWeight, py::arg("lw"))
		.def("isPlottable", &PyDbLayerTableRecord::isPlottable)
		.def("setIsPlottable", &PyDbLayerTableRecord::setIsPlottable, py::arg("plot"))
		.def("plotStyleName", &PyDbLayerTableRecord::plotStyleName)
		.def("plotStyleNameId", &PyDbLayerTableRecord::plotStyleNameId)
		.def("setPlotStyleName", &PyDbLayerTableRecord::setPlotStyleName, py::arg("newName"))
		.def("setPlotStyleNameById", &PyDbLayerTableRecord::setPlotStyleNameById, py::arg("newId"))
		.def("isInUse", &PyDbLayerTableRecord::isInUse)
		.def("description", &PyDbLayerTableRecord::description)
		.def("setDescription", &PyDbLayerTableRecord::setDescription, py::arg("description"))
		.def("isHidden", py::overload_cast<>(&PyDbLayerTableRecord::isHidden, py::const_))
		.def("setIsHidden", &PyDbLayerTableRecord::setIsHidden, py::arg("on"))
		// 静态重载改名为 isHiddenByObjectId，避免与实例方法同名触发 pybind 重载限制
		.def_static("isHiddenByObjectId", static_cast<bool(*)(uint64_t)>(&PyDbLayerTableRecord::isHidden), py::arg("objectId"))
		.def("removeColorOverride", &PyDbLayerTableRecord::removeColorOverride, py::arg("viewportId"))
		.def("removeLinetypeOverride", &PyDbLayerTableRecord::removeLinetypeOverride, py::arg("viewportId"))
		.def("removeLineWeightOverride", &PyDbLayerTableRecord::removeLineWeightOverride, py::arg("viewportId"))
		.def("removePlotStyleOverride", &PyDbLayerTableRecord::removePlotStyleOverride, py::arg("viewportId"))
		.def("removeTransparencyOverride", &PyDbLayerTableRecord::removeTransparencyOverride, py::arg("viewportId"))
		.def("removeViewportOverrides", &PyDbLayerTableRecord::removeViewportOverrides, py::arg("viewportId"))
		.def("removeAllOverrides", &PyDbLayerTableRecord::removeAllOverrides)
		.def("hasOverrides", &PyDbLayerTableRecord::hasOverrides, py::arg("viewportId"))
		.def("hasAnyOverrides", &PyDbLayerTableRecord::hasAnyOverrides)
		.def("isReconciled", py::overload_cast<>(&PyDbLayerTableRecord::isReconciled, py::const_))
		.def("setIsReconciled", &PyDbLayerTableRecord::setIsReconciled, py::arg("bReconcile") = true)
		// 静态重载改名为 isReconciledByObjectId，避免与实例方法同名触发 pybind 重载限制
		.def_static("isReconciledByObjectId", static_cast<bool(*)(uint64_t)>(&PyDbLayerTableRecord::isReconciled), py::arg("objectId"))
		// ---- 颜色 / 透明度 ----
		.def("color", &PyDbLayerTableRecord::color, "返回 (colorIndex, r, g, b)；256=ByLayer,0=ByBlock")
		.def("setColor", &PyDbLayerTableRecord::setColor, py::arg("r"), py::arg("g"), py::arg("b"), "以真彩色 RGB 设置层颜色")
		.def("entityColor", &PyDbLayerTableRecord::entityColor, "返回实体颜色 (colorIndex, r, g, b)")
		.def("transparency", &PyDbLayerTableRecord::transparency, "返回 0=ByLayer,1=ByBlock,否则 alpha 0..255")
		.def("setTransparency", &PyDbLayerTableRecord::setTransparency, py::arg("trans"), "0=ByLayer,1=ByBlock,否则 alpha 0..255")
		;
}
