#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbViewport.h"

PyWrapper_Cpp(DbViewport)
AcString PyDbViewport::toString() const { return AcString(std::format("AcDbViewport[wrapper]").c_str(), AcString::Utf8); }
double PyDbViewport::height() const { return impObj()->height(); }

bool  PyDbViewport::setHeight(double h) { return PyWarnIfError(impObj()->setHeight(h), L"AcDbViewport::setHeight"); }

double PyDbViewport::width() const { return impObj()->width(); }

bool  PyDbViewport::setWidth(double w) { return PyWarnIfError(impObj()->setWidth(w), L"AcDbViewport::setWidth"); }

AcGePoint3d PyDbViewport::centerPoint() const { return impObj()->centerPoint(); }

bool  PyDbViewport::setCenterPoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setCenterPoint(p), L"AcDbViewport::setCenterPoint"); }

AcGePoint3d PyDbViewport::viewTarget() const { return impObj()->viewTarget(); }

bool  PyDbViewport::setViewTarget(const AcGePoint3d& t) { return PyWarnIfError(impObj()->setViewTarget(t), L"AcDbViewport::setViewTarget"); }

int PyDbViewport::number() const { return impObj()->number(); }
bool PyDbViewport::isOn() const { return impObj()->isOn(); }
bool  PyDbViewport::setIsOn(bool v) { return PyWarnIfError(impObj()->setIsOn(v), L"AcDbViewport::setIsOn"); }
AcGeVector3d PyDbViewport::viewDirection() const { return impObj()->viewDirection(); }
bool  PyDbViewport::setViewDirection(const AcGeVector3d& v) { return PyWarnIfError(impObj()->setViewDirection(v), L"AcDbViewport::setViewDirection"); }
double PyDbViewport::viewHeight() const { return impObj()->viewHeight(); }
bool  PyDbViewport::setViewHeight(double v) { return PyWarnIfError(impObj()->setViewHeight(v), L"AcDbViewport::setViewHeight"); }
AcGePoint2d PyDbViewport::viewCenter() const { return impObj()->viewCenter(); }
bool  PyDbViewport::setViewCenter(const AcGePoint2d& p) { return PyWarnIfError(impObj()->setViewCenter(p), L"AcDbViewport::setViewCenter"); }
double PyDbViewport::twistAngle() const { return impObj()->twistAngle(); }
bool  PyDbViewport::setTwistAngle(double v) { return PyWarnIfError(impObj()->setTwistAngle(v), L"AcDbViewport::setTwistAngle"); }
double PyDbViewport::lensLength() const { return impObj()->lensLength(); }
bool  PyDbViewport::setLensLength(double v) { return PyWarnIfError(impObj()->setLensLength(v), L"AcDbViewport::setLensLength"); }
bool PyDbViewport::isFrontClipOn() const { return impObj()->isFrontClipOn(); }
bool  PyDbViewport::setFrontClipOn(bool v) { return PyWarnIfError(impObj()->setFrontClipOn(v), L"AcDbViewport::setFrontClipOn"); }
bool PyDbViewport::isBackClipOn() const { return impObj()->isBackClipOn(); }
bool  PyDbViewport::setBackClipOn(bool v) { return PyWarnIfError(impObj()->setBackClipOn(v), L"AcDbViewport::setBackClipOn"); }
double PyDbViewport::frontClipDistance() const { return impObj()->frontClipDistance(); }
bool  PyDbViewport::setFrontClipDistance(double v) { return PyWarnIfError(impObj()->setFrontClipDistance(v), L"AcDbViewport::setFrontClipDistance"); }
double PyDbViewport::backClipDistance() const { return impObj()->backClipDistance(); }
bool  PyDbViewport::setBackClipDistance(double v) { return PyWarnIfError(impObj()->setBackClipDistance(v), L"AcDbViewport::setBackClipDistance"); }
bool PyDbViewport::isPerspectiveOn() const { return impObj()->isPerspectiveOn(); }
bool  PyDbViewport::setPerspectiveOn(bool v) { return PyWarnIfError(impObj()->setPerspectiveOn(v), L"AcDbViewport::setPerspectiveOn"); }
int PyDbViewport::circleSides() const { return impObj()->circleSides(); }
bool  PyDbViewport::setCircleSides(int v) { return PyWarnIfError(impObj()->setCircleSides(static_cast<Adesk::UInt16>(v)), L"AcDbViewport::setCircleSides"); }
bool PyDbViewport::isSnapOn() const { return impObj()->isSnapOn(); }
bool  PyDbViewport::setSnapOn(bool v) { return PyWarnIfError(impObj()->setSnapOn(v), L"AcDbViewport::setSnapOn"); }
double PyDbViewport::snapAngle() const { return impObj()->snapAngle(); }
bool  PyDbViewport::setSnapAngle(double v) { return PyWarnIfError(impObj()->setSnapAngle(v), L"AcDbViewport::setSnapAngle"); }
AcGePoint2d PyDbViewport::snapBasePoint() const { return impObj()->snapBasePoint(); }
bool  PyDbViewport::setSnapBasePoint(const AcGePoint2d& p) { return PyWarnIfError(impObj()->setSnapBasePoint(p), L"AcDbViewport::setSnapBasePoint"); }
AcGeVector2d PyDbViewport::snapIncrement() const { return impObj()->snapIncrement(); }
bool  PyDbViewport::setSnapIncrement(const AcGeVector2d& v) { return PyWarnIfError(impObj()->setSnapIncrement(v), L"AcDbViewport::setSnapIncrement"); }
bool PyDbViewport::isGridOn() const { return impObj()->isGridOn(); }
bool  PyDbViewport::setGridOn(bool v) { return PyWarnIfError(impObj()->setGridOn(v), L"AcDbViewport::setGridOn"); }
int PyDbViewport::gridMajor() const { return impObj()->gridMajor(); }
bool  PyDbViewport::setGridMajor(int v) { return PyWarnIfError(impObj()->setGridMajor(static_cast<Adesk::UInt16>(v)), L"AcDbViewport::setGridMajor"); }
AcGeVector2d PyDbViewport::gridIncrement() const { return impObj()->gridIncrement(); }
bool  PyDbViewport::setGridIncrement(const AcGeVector2d& v) { return PyWarnIfError(impObj()->setGridIncrement(v), L"AcDbViewport::setGridIncrement"); }
bool PyDbViewport::hiddenLinesRemoved() const { return impObj()->hiddenLinesRemoved(); }
bool  PyDbViewport::removeHiddenLines(bool v) { return PyWarnIfError(impObj()->removeHiddenLines(v), L"AcDbViewport::removeHiddenLines"); }
bool  PyDbViewport::updateDisplay() { return PyWarnIfError(impObj()->updateDisplay(), L"AcDbViewport::updateDisplay"); }
bool PyDbViewport::isLocked() const { return impObj()->isLocked(); }
bool  PyDbViewport::setLocked() { return PyWarnIfError(impObj()->setLocked(), L"AcDbViewport::setLocked"); }
bool  PyDbViewport::setUnlocked() { return PyWarnIfError(impObj()->setUnlocked(), L"AcDbViewport::setUnlocked"); }

// ---- 显示/裁剪/UCS 开关与视图关联（AcDbViewport 自己）----
bool PyDbViewport::isFastZoomOn() const { return impObj()->isFastZoomOn(); }
bool PyDbViewport::isFrontClipAtEyeOn() const { return impObj()->isFrontClipAtEyeOn(); }
bool PyDbViewport::isSnapIsometric() const { return impObj()->isSnapIsometric(); }
bool PyDbViewport::isUcsFollowModeOn() const { return impObj()->isUcsFollowModeOn(); }
bool PyDbViewport::isUcsIconAtOrigin() const { return impObj()->isUcsIconAtOrigin(); }
bool PyDbViewport::isUcsIconVisible() const { return impObj()->isUcsIconVisible(); }

uint64_t PyDbViewport::getLabelBlock() const {
	AcDbObjectId id;
	if(!PyWarnIfError(impObj()->getLabelBlock(id), L"AcDbViewport::getLabelBlock")) return 0ULL;
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}
uint64_t PyDbViewport::getSheetView() const {
	AcDbObjectId id;
	if(!PyWarnIfError(impObj()->getSheetView(id), L"AcDbViewport::getSheetView")) return 0ULL;
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}

bool  PyDbViewport::removeLabelBlock() { return PyWarnIfError(impObj()->removeLabelBlock(), L"AcDbViewport::removeLabelBlock"); }
bool  PyDbViewport::removeModelView() { return PyWarnIfError(impObj()->removeModelView(), L"AcDbViewport::removeModelView"); }
bool  PyDbViewport::removeSheetView() { return PyWarnIfError(impObj()->removeSheetView(), L"AcDbViewport::removeSheetView"); }
bool  PyDbViewport::setBackClipOff() { return PyWarnIfError(impObj()->setBackClipOff(), L"AcDbViewport::setBackClipOff"); }
bool  PyDbViewport::setFastZoomOff() { return PyWarnIfError(impObj()->setFastZoomOff(), L"AcDbViewport::setFastZoomOff"); }
bool  PyDbViewport::setFastZoomOn(bool bOn) { return PyWarnIfError(impObj()->setFastZoomOn(bOn), L"AcDbViewport::setFastZoomOn"); }
bool  PyDbViewport::setFrontClipAtEyeOff() { return PyWarnIfError(impObj()->setFrontClipAtEyeOff(), L"AcDbViewport::setFrontClipAtEyeOff"); }
bool  PyDbViewport::setFrontClipAtEyeOn(bool bOn) { return PyWarnIfError(impObj()->setFrontClipAtEyeOn(bOn), L"AcDbViewport::setFrontClipAtEyeOn"); }
bool  PyDbViewport::setFrontClipOff() { return PyWarnIfError(impObj()->setFrontClipOff(), L"AcDbViewport::setFrontClipOff"); }
bool  PyDbViewport::setLabelBlock(uint64_t objId) {
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(objId));
	return PyWarnIfError(impObj()->setLabelBlock(id), L"AcDbViewport::setLabelBlock");
}
bool  PyDbViewport::setOff() { return PyWarnIfError(impObj()->setOff(), L"AcDbViewport::setOff"); }
bool  PyDbViewport::setOn() { return PyWarnIfError(impObj()->setOn(), L"AcDbViewport::setOn"); }
bool  PyDbViewport::setPerspectiveOff() { return PyWarnIfError(impObj()->setPerspectiveOff(), L"AcDbViewport::setPerspectiveOff"); }
bool  PyDbViewport::setSheetView(uint64_t objId) {
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(objId));
	return PyWarnIfError(impObj()->setSheetView(id), L"AcDbViewport::setSheetView");
}
bool  PyDbViewport::setSnapIsometric(bool bOn) { return PyWarnIfError(impObj()->setSnapIsometric(bOn), L"AcDbViewport::setSnapIsometric"); }
bool  PyDbViewport::setSnapOff() { return PyWarnIfError(impObj()->setSnapOff(), L"AcDbViewport::setSnapOff"); }
bool  PyDbViewport::setUcsFollowModeOff() { return PyWarnIfError(impObj()->setUcsFollowModeOff(), L"AcDbViewport::setUcsFollowModeOff"); }
bool  PyDbViewport::setUcsFollowModeOn(bool bOn) { return PyWarnIfError(impObj()->setUcsFollowModeOn(bOn), L"AcDbViewport::setUcsFollowModeOn"); }
bool  PyDbViewport::setUcsIconAtCorner() { return PyWarnIfError(impObj()->setUcsIconAtCorner(), L"AcDbViewport::setUcsIconAtCorner"); }
bool  PyDbViewport::setUcsIconAtOrigin(bool bOn) { return PyWarnIfError(impObj()->setUcsIconAtOrigin(bOn), L"AcDbViewport::setUcsIconAtOrigin"); }
bool  PyDbViewport::setUcsIconInvisible() { return PyWarnIfError(impObj()->setUcsIconInvisible(), L"AcDbViewport::setUcsIconInvisible"); }
bool  PyDbViewport::setUcsIconVisible(bool bOn) { return PyWarnIfError(impObj()->setUcsIconVisible(bOn), L"AcDbViewport::setUcsIconVisible"); }
bool  PyDbViewport::syncModelView() { return PyWarnIfError(impObj()->syncModelView(), L"AcDbViewport::syncModelView"); }

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
		;
}

