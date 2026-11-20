#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbViewport.h"

PyWrapper_Cpp(DbViewport)
AcString PyDbViewport::toString() const { return AcString(std::format("AcDbViewport[wrapper]").c_str(), AcString::Utf8); }
double PyDbViewport::height() const { if (!impObj()) return 0.0; return impObj() ? impObj()->height() : 0.0; }

bool  PyDbViewport::setHeight(double h) { if (!impObj()) return false; return PyWarnIfError(impObj()->setHeight(h), L"AcDbViewport::setHeight"); }

double PyDbViewport::width() const { if (!impObj()) return 0.0; return impObj() ? impObj()->width() : 0.0; }

bool  PyDbViewport::setWidth(double w) { if (!impObj()) return false; return PyWarnIfError(impObj()->setWidth(w), L"AcDbViewport::setWidth"); }

AcGePoint3d PyDbViewport::centerPoint() const { if (!impObj()) return AcGePoint3d(); return impObj() ? impObj()->centerPoint() : AcGePoint3d(); }

bool  PyDbViewport::setCenterPoint(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setCenterPoint(p), L"AcDbViewport::setCenterPoint"); }

AcGePoint3d PyDbViewport::viewTarget() const { if (!impObj()) return AcGePoint3d(); return impObj() ? impObj()->viewTarget() : AcGePoint3d(); }

bool  PyDbViewport::setViewTarget(const AcGePoint3d& t) { if (!impObj()) return false; return PyWarnIfError(impObj()->setViewTarget(t), L"AcDbViewport::setViewTarget"); }

int PyDbViewport::number() const { if (!impObj()) return 0; return impObj() ? impObj()->number() : 0; }
bool PyDbViewport::isOn() const { if (!impObj()) return false; return impObj() ? impObj()->isOn() : false; }
bool  PyDbViewport::setIsOn(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setIsOn(v), L"AcDbViewport::setIsOn"); }
AcGeVector3d PyDbViewport::viewDirection() const { if (!impObj()) return AcGeVector3d(); return impObj() ? impObj()->viewDirection() : AcGeVector3d(); }
bool  PyDbViewport::setViewDirection(const AcGeVector3d& v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setViewDirection(v), L"AcDbViewport::setViewDirection"); }
double PyDbViewport::viewHeight() const { if (!impObj()) return 0.0; return impObj() ? impObj()->viewHeight() : 0.0; }
bool  PyDbViewport::setViewHeight(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setViewHeight(v), L"AcDbViewport::setViewHeight"); }
AcGePoint2d PyDbViewport::viewCenter() const { if (!impObj()) return AcGePoint2d(); return impObj() ? impObj()->viewCenter() : AcGePoint2d(); }
bool  PyDbViewport::setViewCenter(const AcGePoint2d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setViewCenter(p), L"AcDbViewport::setViewCenter"); }
double PyDbViewport::twistAngle() const { if (!impObj()) return 0.0; return impObj() ? impObj()->twistAngle() : 0.0; }
bool  PyDbViewport::setTwistAngle(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setTwistAngle(v), L"AcDbViewport::setTwistAngle"); }
double PyDbViewport::lensLength() const { if (!impObj()) return 0.0; return impObj() ? impObj()->lensLength() : 0.0; }
bool  PyDbViewport::setLensLength(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setLensLength(v), L"AcDbViewport::setLensLength"); }
bool PyDbViewport::isFrontClipOn() const { if (!impObj()) return false; return impObj() ? impObj()->isFrontClipOn() : false; }
bool  PyDbViewport::setFrontClipOn(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setFrontClipOn(v), L"AcDbViewport::setFrontClipOn"); }
bool PyDbViewport::isBackClipOn() const { if (!impObj()) return false; return impObj() ? impObj()->isBackClipOn() : false; }
bool  PyDbViewport::setBackClipOn(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setBackClipOn(v), L"AcDbViewport::setBackClipOn"); }
double PyDbViewport::frontClipDistance() const { if (!impObj()) return 0.0; return impObj() ? impObj()->frontClipDistance() : 0.0; }
bool  PyDbViewport::setFrontClipDistance(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setFrontClipDistance(v), L"AcDbViewport::setFrontClipDistance"); }
double PyDbViewport::backClipDistance() const { if (!impObj()) return 0.0; return impObj() ? impObj()->backClipDistance() : 0.0; }
bool  PyDbViewport::setBackClipDistance(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setBackClipDistance(v), L"AcDbViewport::setBackClipDistance"); }
bool PyDbViewport::isPerspectiveOn() const { if (!impObj()) return false; return impObj() ? impObj()->isPerspectiveOn() : false; }
bool  PyDbViewport::setPerspectiveOn(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setPerspectiveOn(v), L"AcDbViewport::setPerspectiveOn"); }
int PyDbViewport::circleSides() const { if (!impObj()) return 0; return impObj() ? impObj()->circleSides() : 0; }
bool  PyDbViewport::setCircleSides(int v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setCircleSides(static_cast<Adesk::UInt16>(v)), L"AcDbViewport::setCircleSides"); }
bool PyDbViewport::isSnapOn() const { if (!impObj()) return false; return impObj() ? impObj()->isSnapOn() : false; }
bool  PyDbViewport::setSnapOn(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setSnapOn(v), L"AcDbViewport::setSnapOn"); }
double PyDbViewport::snapAngle() const { if (!impObj()) return 0.0; return impObj() ? impObj()->snapAngle() : 0.0; }
bool  PyDbViewport::setSnapAngle(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setSnapAngle(v), L"AcDbViewport::setSnapAngle"); }
AcGePoint2d PyDbViewport::snapBasePoint() const { if (!impObj()) return AcGePoint2d(); return impObj() ? impObj()->snapBasePoint() : AcGePoint2d(); }
bool  PyDbViewport::setSnapBasePoint(const AcGePoint2d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setSnapBasePoint(p), L"AcDbViewport::setSnapBasePoint"); }
AcGeVector2d PyDbViewport::snapIncrement() const { if (!impObj()) return AcGeVector2d(); return impObj() ? impObj()->snapIncrement() : AcGeVector2d(); }
bool  PyDbViewport::setSnapIncrement(const AcGeVector2d& v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setSnapIncrement(v), L"AcDbViewport::setSnapIncrement"); }
bool PyDbViewport::isGridOn() const { if (!impObj()) return false; return impObj() ? impObj()->isGridOn() : false; }
bool  PyDbViewport::setGridOn(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setGridOn(v), L"AcDbViewport::setGridOn"); }
int PyDbViewport::gridMajor() const { if (!impObj()) return 0; return impObj() ? impObj()->gridMajor() : 0; }
bool  PyDbViewport::setGridMajor(int v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setGridMajor(static_cast<Adesk::UInt16>(v)), L"AcDbViewport::setGridMajor"); }
AcGeVector2d PyDbViewport::gridIncrement() const { if (!impObj()) return AcGeVector2d(); return impObj() ? impObj()->gridIncrement() : AcGeVector2d(); }
bool  PyDbViewport::setGridIncrement(const AcGeVector2d& v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setGridIncrement(v), L"AcDbViewport::setGridIncrement"); }
bool PyDbViewport::hiddenLinesRemoved() const { if (!impObj()) return false; return impObj() ? impObj()->hiddenLinesRemoved() : false; }
bool  PyDbViewport::removeHiddenLines(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->removeHiddenLines(v), L"AcDbViewport::removeHiddenLines"); }
bool  PyDbViewport::updateDisplay() { if (!impObj()) return false; return PyWarnIfError(impObj()->updateDisplay(), L"AcDbViewport::updateDisplay"); }
bool PyDbViewport::isLocked() const { if (!impObj()) return false; return impObj() ? impObj()->isLocked() : false; }
bool  PyDbViewport::setLocked() { if (!impObj()) return false; return PyWarnIfError(impObj()->setLocked(), L"AcDbViewport::setLocked"); }
bool  PyDbViewport::setUnlocked() { if (!impObj()) return false; return PyWarnIfError(impObj()->setUnlocked(), L"AcDbViewport::setUnlocked"); }

// ---- 显示/裁剪/UCS 开关与视图关联（AcDbViewport 自己）----
bool PyDbViewport::isFastZoomOn() const { if (!impObj()) return false; return impObj() ? impObj()->isFastZoomOn() : false; }
bool PyDbViewport::isFrontClipAtEyeOn() const { if (!impObj()) return false; return impObj() ? impObj()->isFrontClipAtEyeOn() : false; }
bool PyDbViewport::isSnapIsometric() const { if (!impObj()) return false; return impObj() ? impObj()->isSnapIsometric() : false; }
bool PyDbViewport::isUcsFollowModeOn() const { if (!impObj()) return false; return impObj() ? impObj()->isUcsFollowModeOn() : false; }
bool PyDbViewport::isUcsIconAtOrigin() const { if (!impObj()) return false; return impObj() ? impObj()->isUcsIconAtOrigin() : false; }
bool PyDbViewport::isUcsIconVisible() const { if (!impObj()) return false; return impObj() ? impObj()->isUcsIconVisible() : false; }

uint64_t PyDbViewport::getLabelBlock() const {
	if (!impObj()) return 0ULL;
	AcDbObjectId id;
	if(!PyWarnIfError(impObj()->getLabelBlock(id), L"AcDbViewport::getLabelBlock")) return 0ULL;
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}
uint64_t PyDbViewport::getSheetView() const {
	if (!impObj()) return 0ULL;
	AcDbObjectId id;
	if(!PyWarnIfError(impObj()->getSheetView(id), L"AcDbViewport::getSheetView")) return 0ULL;
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}

bool  PyDbViewport::removeLabelBlock() { if (!impObj()) return false; return PyWarnIfError(impObj()->removeLabelBlock(), L"AcDbViewport::removeLabelBlock"); }
bool  PyDbViewport::removeModelView() { if (!impObj()) return false; return PyWarnIfError(impObj()->removeModelView(), L"AcDbViewport::removeModelView"); }
bool  PyDbViewport::removeSheetView() { if (!impObj()) return false; return PyWarnIfError(impObj()->removeSheetView(), L"AcDbViewport::removeSheetView"); }
bool  PyDbViewport::setBackClipOff() { if (!impObj()) return false; return PyWarnIfError(impObj()->setBackClipOff(), L"AcDbViewport::setBackClipOff"); }
bool  PyDbViewport::setFastZoomOff() { if (!impObj()) return false; return PyWarnIfError(impObj()->setFastZoomOff(), L"AcDbViewport::setFastZoomOff"); }
bool  PyDbViewport::setFastZoomOn(bool bOn) { if (!impObj()) return false; return PyWarnIfError(impObj()->setFastZoomOn(bOn), L"AcDbViewport::setFastZoomOn"); }
bool  PyDbViewport::setFrontClipAtEyeOff() { if (!impObj()) return false; return PyWarnIfError(impObj()->setFrontClipAtEyeOff(), L"AcDbViewport::setFrontClipAtEyeOff"); }
bool  PyDbViewport::setFrontClipAtEyeOn(bool bOn) { if (!impObj()) return false; return PyWarnIfError(impObj()->setFrontClipAtEyeOn(bOn), L"AcDbViewport::setFrontClipAtEyeOn"); }
bool  PyDbViewport::setFrontClipOff() { if (!impObj()) return false; return PyWarnIfError(impObj()->setFrontClipOff(), L"AcDbViewport::setFrontClipOff"); }
bool  PyDbViewport::setLabelBlock(uint64_t objId) {
	if (!impObj()) return false;
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(objId));
	return PyWarnIfError(impObj()->setLabelBlock(id), L"AcDbViewport::setLabelBlock");
}
bool  PyDbViewport::setOff() { if (!impObj()) return false; return PyWarnIfError(impObj()->setOff(), L"AcDbViewport::setOff"); }
bool  PyDbViewport::setOn() { if (!impObj()) return false; return PyWarnIfError(impObj()->setOn(), L"AcDbViewport::setOn"); }
bool  PyDbViewport::setPerspectiveOff() { if (!impObj()) return false; return PyWarnIfError(impObj()->setPerspectiveOff(), L"AcDbViewport::setPerspectiveOff"); }
bool  PyDbViewport::setSheetView(uint64_t objId) {
	if (!impObj()) return false;
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(objId));
	return PyWarnIfError(impObj()->setSheetView(id), L"AcDbViewport::setSheetView");
}
bool  PyDbViewport::setSnapIsometric(bool bOn) { if (!impObj()) return false; return PyWarnIfError(impObj()->setSnapIsometric(bOn), L"AcDbViewport::setSnapIsometric"); }
bool  PyDbViewport::setSnapOff() { if (!impObj()) return false; return PyWarnIfError(impObj()->setSnapOff(), L"AcDbViewport::setSnapOff"); }
bool  PyDbViewport::setUcsFollowModeOff() { if (!impObj()) return false; return PyWarnIfError(impObj()->setUcsFollowModeOff(), L"AcDbViewport::setUcsFollowModeOff"); }
bool  PyDbViewport::setUcsFollowModeOn(bool bOn) { if (!impObj()) return false; return PyWarnIfError(impObj()->setUcsFollowModeOn(bOn), L"AcDbViewport::setUcsFollowModeOn"); }
bool  PyDbViewport::setUcsIconAtCorner() { if (!impObj()) return false; return PyWarnIfError(impObj()->setUcsIconAtCorner(), L"AcDbViewport::setUcsIconAtCorner"); }
bool  PyDbViewport::setUcsIconAtOrigin(bool bOn) { if (!impObj()) return false; return PyWarnIfError(impObj()->setUcsIconAtOrigin(bOn), L"AcDbViewport::setUcsIconAtOrigin"); }
bool  PyDbViewport::setUcsIconInvisible() { if (!impObj()) return false; return PyWarnIfError(impObj()->setUcsIconInvisible(), L"AcDbViewport::setUcsIconInvisible"); }
bool  PyDbViewport::setUcsIconVisible(bool bOn) { if (!impObj()) return false; return PyWarnIfError(impObj()->setUcsIconVisible(bOn), L"AcDbViewport::setUcsIconVisible"); }
bool  PyDbViewport::syncModelView() { if (!impObj()) return false; return PyWarnIfError(impObj()->syncModelView(), L"AcDbViewport::syncModelView"); }

void bind_PyDbViewport(pym& m) {
	py::class_<PyDbViewport, PyDbEntity>(m, "PyDbViewport")
		.def(py::init<>())
		.def(py::init<AcDbViewport*>(), py::arg("object"))
		.def("height", &PyDbViewport::height)
		.def("setHeight", &PyDbViewport::setHeight, py::arg("h"))
		.def("width", &PyDbViewport::width)
		.def("setWidth", &PyDbViewport::setWidth, py::arg("w"))
		.def("centerPoint", &PyDbViewport::centerPoint)
		.def("setCenterPoint", &PyDbViewport::setCenterPoint, py::arg("p"))
		.def("viewTarget", &PyDbViewport::viewTarget)
		.def("setViewTarget", &PyDbViewport::setViewTarget, py::arg("t"))
		.def("number", &PyDbViewport::number)
		.def("isOn", &PyDbViewport::isOn)
		.def("setIsOn", &PyDbViewport::setIsOn, py::arg("v"))
		.def("viewDirection", &PyDbViewport::viewDirection)
		.def("setViewDirection", &PyDbViewport::setViewDirection, py::arg("v"))
		.def("viewHeight", &PyDbViewport::viewHeight)
		.def("setViewHeight", &PyDbViewport::setViewHeight, py::arg("v"))
		.def("viewCenter", &PyDbViewport::viewCenter)
		.def("setViewCenter", &PyDbViewport::setViewCenter, py::arg("p"))
		.def("twistAngle", &PyDbViewport::twistAngle)
		.def("setTwistAngle", &PyDbViewport::setTwistAngle, py::arg("v"))
		.def("lensLength", &PyDbViewport::lensLength)
		.def("setLensLength", &PyDbViewport::setLensLength, py::arg("v"))
		.def("isFrontClipOn", &PyDbViewport::isFrontClipOn)
		.def("setFrontClipOn", &PyDbViewport::setFrontClipOn, py::arg("v"))
		.def("isBackClipOn", &PyDbViewport::isBackClipOn)
		.def("setBackClipOn", &PyDbViewport::setBackClipOn, py::arg("v"))
		.def("frontClipDistance", &PyDbViewport::frontClipDistance)
		.def("setFrontClipDistance", &PyDbViewport::setFrontClipDistance, py::arg("v"))
		.def("backClipDistance", &PyDbViewport::backClipDistance)
		.def("setBackClipDistance", &PyDbViewport::setBackClipDistance, py::arg("v"))
		.def("isPerspectiveOn", &PyDbViewport::isPerspectiveOn)
		.def("setPerspectiveOn", &PyDbViewport::setPerspectiveOn, py::arg("v"))
		.def("circleSides", &PyDbViewport::circleSides)
		.def("setCircleSides", &PyDbViewport::setCircleSides, py::arg("v"))
		.def("isSnapOn", &PyDbViewport::isSnapOn)
		.def("setSnapOn", &PyDbViewport::setSnapOn, py::arg("v"))
		.def("snapAngle", &PyDbViewport::snapAngle)
		.def("setSnapAngle", &PyDbViewport::setSnapAngle, py::arg("v"))
		.def("snapBasePoint", &PyDbViewport::snapBasePoint)
		.def("setSnapBasePoint", &PyDbViewport::setSnapBasePoint, py::arg("p"))
		.def("snapIncrement", &PyDbViewport::snapIncrement)
		.def("setSnapIncrement", &PyDbViewport::setSnapIncrement, py::arg("v"))
		.def("isGridOn", &PyDbViewport::isGridOn)
		.def("setGridOn", &PyDbViewport::setGridOn, py::arg("v"))
		.def("gridMajor", &PyDbViewport::gridMajor)
		.def("setGridMajor", &PyDbViewport::setGridMajor, py::arg("v"))
		.def("gridIncrement", &PyDbViewport::gridIncrement)
		.def("setGridIncrement", &PyDbViewport::setGridIncrement, py::arg("v"))
		.def("hiddenLinesRemoved", &PyDbViewport::hiddenLinesRemoved)
		.def("removeHiddenLines", &PyDbViewport::removeHiddenLines, py::arg("v"))
		.def("updateDisplay", &PyDbViewport::updateDisplay)
		.def("isLocked", &PyDbViewport::isLocked)
		.def("setLocked", &PyDbViewport::setLocked)
		.def("setUnlocked", &PyDbViewport::setUnlocked)
		// ---- 显示/裁剪/UCS 开关与视图关联 ----
		.def("isFastZoomOn", &PyDbViewport::isFastZoomOn)
		.def("isFrontClipAtEyeOn", &PyDbViewport::isFrontClipAtEyeOn)
		.def("isSnapIsometric", &PyDbViewport::isSnapIsometric)
		.def("isUcsFollowModeOn", &PyDbViewport::isUcsFollowModeOn)
		.def("isUcsIconAtOrigin", &PyDbViewport::isUcsIconAtOrigin)
		.def("isUcsIconVisible", &PyDbViewport::isUcsIconVisible)
		.def("getLabelBlock", &PyDbViewport::getLabelBlock, "取标签块 ObjectId（uint64），无标签块返回 0")
		.def("getSheetView", &PyDbViewport::getSheetView, "取图纸视图 ObjectId（uint64），无关联返回 0")
		.def("removeLabelBlock", &PyDbViewport::removeLabelBlock)
		.def("removeModelView", &PyDbViewport::removeModelView)
		.def("removeSheetView", &PyDbViewport::removeSheetView)
		.def("setBackClipOff", &PyDbViewport::setBackClipOff)
		.def("setFastZoomOff", &PyDbViewport::setFastZoomOff)
		.def("setFastZoomOn", &PyDbViewport::setFastZoomOn, py::arg("bOn") = true)
		.def("setFrontClipAtEyeOff", &PyDbViewport::setFrontClipAtEyeOff)
		.def("setFrontClipAtEyeOn", &PyDbViewport::setFrontClipAtEyeOn, py::arg("bOn") = true)
		.def("setFrontClipOff", &PyDbViewport::setFrontClipOff)
		.def("setLabelBlock", &PyDbViewport::setLabelBlock, py::arg("objId"), "objId 为 uint64 ObjectId（asOldId）")
		.def("setOff", &PyDbViewport::setOff)
		.def("setOn", &PyDbViewport::setOn)
		.def("setPerspectiveOff", &PyDbViewport::setPerspectiveOff)
		.def("setSheetView", &PyDbViewport::setSheetView, py::arg("objId"), "objId 为 uint64 ObjectId（asOldId）")
		.def("setSnapIsometric", &PyDbViewport::setSnapIsometric, py::arg("bOn") = true)
		.def("setSnapOff", &PyDbViewport::setSnapOff)
		.def("setUcsFollowModeOff", &PyDbViewport::setUcsFollowModeOff)
		.def("setUcsFollowModeOn", &PyDbViewport::setUcsFollowModeOn, py::arg("bOn") = true)
		.def("setUcsIconAtCorner", &PyDbViewport::setUcsIconAtCorner)
		.def("setUcsIconAtOrigin", &PyDbViewport::setUcsIconAtOrigin, py::arg("bOn") = true)
		.def("setUcsIconInvisible", &PyDbViewport::setUcsIconInvisible)
		.def("setUcsIconVisible", &PyDbViewport::setUcsIconVisible, py::arg("bOn") = true)
		.def("syncModelView", &PyDbViewport::syncModelView)
		PyWrapper_BIND(DbViewport)
		.def("__repr__", &PyDbViewport::toString)
		// ---- PyRx getter 对齐补齐（自动生成，纯读无副作用） ----
		.def("background", [](const PyDbViewport& o) -> uint64_t { if (!o.impObj()) return 0ULL; auto id = o.impObj()->background(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("brightness", [](const PyDbViewport& o) -> double { if (!o.impObj()) return 0.0; return o.impObj()->brightness(); })
		.def("contrast", [](const PyDbViewport& o) -> double { if (!o.impObj()) return 0.0; return o.impObj()->contrast(); })
		.def("customScale", [](const PyDbViewport& o) -> double { if (!o.impObj()) return 0.0; return o.impObj()->customScale(); })
		.def("elevation", [](const PyDbViewport& o) -> double { if (!o.impObj()) return 0.0; return o.impObj()->elevation(); })
		.def("isDefaultLightingOn", [](const PyDbViewport& o) -> bool { if (!o.impObj()) return false; return o.impObj()->isDefaultLightingOn(); })
		.def("isGridAdaptive", [](const PyDbViewport& o) -> bool { if (!o.impObj()) return false; return o.impObj()->isGridAdaptive(); })
		.def("isGridBoundToLimits", [](const PyDbViewport& o) -> bool { if (!o.impObj()) return false; return o.impObj()->isGridBoundToLimits(); })
		.def("isGridFollow", [](const PyDbViewport& o) -> bool { if (!o.impObj()) return false; return o.impObj()->isGridFollow(); })
		.def("isGridSubdivisionRestricted", [](const PyDbViewport& o) -> bool { if (!o.impObj()) return false; return o.impObj()->isGridSubdivisionRestricted(); })
		.def("isNonRectClipOn", [](const PyDbViewport& o) -> bool { if (!o.impObj()) return false; return o.impObj()->isNonRectClipOn(); })
		.def("isTransparent", [](const PyDbViewport& o) -> bool { if (!o.impObj()) return false; return o.impObj()->isTransparent(); })
		.def("isUcsSavedWithViewport", [](const PyDbViewport& o) -> bool { if (!o.impObj()) return false; return o.impObj()->isUcsSavedWithViewport(); })
		.def("nonRectClipEntityId", [](const PyDbViewport& o) -> uint64_t { if (!o.impObj()) return 0ULL; auto id = o.impObj()->nonRectClipEntityId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("plotAsRaster", [](const PyDbViewport& o) -> bool { if (!o.impObj()) return false; return o.impObj()->plotAsRaster(); })
		.def("plotWireframe", [](const PyDbViewport& o) -> bool { if (!o.impObj()) return false; return o.impObj()->plotWireframe(); })
		.def("previousBackgroundForcedSwitch", [](const PyDbViewport& o) -> bool { if (!o.impObj()) return false; return o.impObj()->previousBackgroundForcedSwitch(); })
		.def("shadePlotId", [](const PyDbViewport& o) -> uint64_t { if (!o.impObj()) return 0ULL; auto id = o.impObj()->shadePlotId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("snapIsoPair", [](const PyDbViewport& o) -> int { if (!o.impObj()) return 0; return static_cast<int>(o.impObj()->snapIsoPair()); })
		.def("sunId", [](const PyDbViewport& o) -> uint64_t { if (!o.impObj()) return 0ULL; auto id = o.impObj()->sunId(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("ucsName", [](const PyDbViewport& o) -> uint64_t { if (!o.impObj()) return 0ULL; auto id = o.impObj()->ucsName(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("visualStyle", [](const PyDbViewport& o) -> uint64_t { if (!o.impObj()) return 0ULL; auto id = o.impObj()->visualStyle(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		;
}

