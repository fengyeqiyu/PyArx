#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbViewportTableRecord.h"

PyWrapper_Cpp(DbViewportTableRecord);

// ---- AcDbViewportTableRecord own ----
int PyDbViewportTableRecord::number() const { return static_cast<int>(impObj()->number()); }
AcGePoint2d PyDbViewportTableRecord::lowerLeftCorner() const { return impObj()->lowerLeftCorner(); }
bool PyDbViewportTableRecord::setLowerLeftCorner(const AcGePoint2d& pt) { impObj()->setLowerLeftCorner(pt); return true; }
AcGePoint2d PyDbViewportTableRecord::upperRightCorner() const { return impObj()->upperRightCorner(); }
bool PyDbViewportTableRecord::setUpperRightCorner(const AcGePoint2d& pt) { impObj()->setUpperRightCorner(pt); return true; }
bool PyDbViewportTableRecord::ucsFollowMode() const { return impObj()->ucsFollowMode(); }
bool PyDbViewportTableRecord::setUcsFollowMode(bool enabled) { impObj()->setUcsFollowMode(enabled); return true; }
int PyDbViewportTableRecord::circleSides() const { return static_cast<int>(impObj()->circleSides()); }
bool PyDbViewportTableRecord::setCircleSides(int sides) { impObj()->setCircleSides(static_cast<Adesk::UInt16>(sides)); return true; }
bool PyDbViewportTableRecord::iconEnabled() const { return impObj()->iconEnabled(); }
bool PyDbViewportTableRecord::setIconEnabled(bool enabled) { impObj()->setIconEnabled(enabled); return true; }
bool PyDbViewportTableRecord::iconAtOrigin() const { return impObj()->iconAtOrigin(); }
bool PyDbViewportTableRecord::setIconAtOrigin(bool atOrigin) { impObj()->setIconAtOrigin(atOrigin); return true; }
bool PyDbViewportTableRecord::gridEnabled() const { return impObj()->gridEnabled(); }
bool PyDbViewportTableRecord::setGridEnabled(bool enabled) { impObj()->setGridEnabled(enabled); return true; }
AcGePoint2d PyDbViewportTableRecord::gridIncrements() const { return impObj()->gridIncrements(); }
bool PyDbViewportTableRecord::setGridIncrements(const AcGePoint2d& pt) { impObj()->setGridIncrements(pt); return true; }
bool PyDbViewportTableRecord::snapEnabled() const { return impObj()->snapEnabled(); }
bool PyDbViewportTableRecord::setSnapEnabled(bool enabled) { impObj()->setSnapEnabled(enabled); return true; }
bool PyDbViewportTableRecord::isometricSnapEnabled() const { return impObj()->isometricSnapEnabled(); }
bool PyDbViewportTableRecord::setIsometricSnapEnabled(bool enabled) { impObj()->setIsometricSnapEnabled(enabled); return true; }
int PyDbViewportTableRecord::snapPair() const { return static_cast<int>(impObj()->snapPair()); }
bool PyDbViewportTableRecord::setSnapPair(int pairType) { impObj()->setSnapPair(static_cast<Adesk::Int16>(pairType)); return true; }
double PyDbViewportTableRecord::snapAngle() const { return impObj()->snapAngle(); }
bool PyDbViewportTableRecord::setSnapAngle(double angle) { impObj()->setSnapAngle(angle); return true; }
AcGePoint2d PyDbViewportTableRecord::snapBase() const { return impObj()->snapBase(); }
bool PyDbViewportTableRecord::setSnapBase(const AcGePoint2d& pt) { impObj()->setSnapBase(pt); return true; }
AcGePoint2d PyDbViewportTableRecord::snapIncrements() const { return impObj()->snapIncrements(); }
bool PyDbViewportTableRecord::setSnapIncrements(const AcGePoint2d& pt) { impObj()->setSnapIncrements(pt); return true; }
bool PyDbViewportTableRecord::isUcsSavedWithViewport() const { return impObj()->isUcsSavedWithViewport(); }
bool PyDbViewportTableRecord::setUcsPerViewport(bool ucsvp) { impObj()->setUcsPerViewport(ucsvp); return true; }
bool PyDbViewportTableRecord::isGridBoundToLimits() const { return impObj()->isGridBoundToLimits(); }
bool PyDbViewportTableRecord::setGridBoundToLimits(bool enabled) { impObj()->setGridBoundToLimits(enabled); return true; }
bool PyDbViewportTableRecord::isGridAdaptive() const { return impObj()->isGridAdaptive(); }
bool PyDbViewportTableRecord::setGridAdaptive(bool enabled) { impObj()->setGridAdaptive(enabled); return true; }
bool PyDbViewportTableRecord::isGridSubdivisionRestricted() const { return impObj()->isGridSubdivisionRestricted(); }
bool PyDbViewportTableRecord::setGridSubdivisionRestricted(bool enabled) { impObj()->setGridSubdivisionRestricted(enabled); return true; }
bool PyDbViewportTableRecord::isGridFollow() const { return impObj()->isGridFollow(); }
bool PyDbViewportTableRecord::setGridFollow(bool enabled) { impObj()->setGridFollow(enabled); return true; }
int PyDbViewportTableRecord::gridMajor() const { return static_cast<int>(impObj()->gridMajor()); }
bool PyDbViewportTableRecord::setGridMajor(int value) { impObj()->setGridMajor(static_cast<Adesk::Int16>(value)); return true; }

// ---- Inherited from AcDbAbstractViewTableRecord ----
AcGePoint2d PyDbViewportTableRecord::centerPoint() const { return impObj()->centerPoint(); }
bool PyDbViewportTableRecord::setCenterPoint(const AcGePoint2d& pt) { impObj()->setCenterPoint(pt); return true; }
double PyDbViewportTableRecord::height() const { return impObj()->height(); }
bool PyDbViewportTableRecord::setHeight(double h) { impObj()->setHeight(h); return true; }
double PyDbViewportTableRecord::width() const { return impObj()->width(); }
bool PyDbViewportTableRecord::setWidth(double w) { impObj()->setWidth(w); return true; }
AcGePoint3d PyDbViewportTableRecord::target() const { return impObj()->target(); }
bool PyDbViewportTableRecord::setTarget(const AcGePoint3d& t) { impObj()->setTarget(t); return true; }
AcGeVector3d PyDbViewportTableRecord::viewDirection() const { return impObj()->viewDirection(); }
bool PyDbViewportTableRecord::setViewDirection(const AcGeVector3d& v) { impObj()->setViewDirection(v); return true; }
double PyDbViewportTableRecord::viewTwist() const { return impObj()->viewTwist(); }
bool PyDbViewportTableRecord::setViewTwist(double angle) { impObj()->setViewTwist(angle); return true; }
double PyDbViewportTableRecord::lensLength() const { return impObj()->lensLength(); }
bool PyDbViewportTableRecord::setLensLength(double length) { impObj()->setLensLength(length); return true; }
bool PyDbViewportTableRecord::perspectiveEnabled() const { return impObj()->perspectiveEnabled(); }
bool PyDbViewportTableRecord::setPerspectiveEnabled(bool enabled) { impObj()->setPerspectiveEnabled(enabled); return true; }

// ---- 裁剪平面与高程（继承自 AcDbAbstractViewTableRecord）----
double PyDbViewportTableRecord::frontClipDistance() const { return impObj()->frontClipDistance(); }
bool PyDbViewportTableRecord::setFrontClipDistance(double d) { impObj()->setFrontClipDistance(d); return true; }
double PyDbViewportTableRecord::backClipDistance() const { return impObj()->backClipDistance(); }
bool PyDbViewportTableRecord::setBackClipDistance(double d) { impObj()->setBackClipDistance(d); return true; }
bool PyDbViewportTableRecord::frontClipEnabled() const { return impObj()->frontClipEnabled(); }
bool PyDbViewportTableRecord::setFrontClipEnabled(bool enabled) { impObj()->setFrontClipEnabled(enabled); return true; }
bool PyDbViewportTableRecord::backClipEnabled() const { return impObj()->backClipEnabled(); }
bool PyDbViewportTableRecord::setBackClipEnabled(bool enabled) { impObj()->setBackClipEnabled(enabled); return true; }
double PyDbViewportTableRecord::elevation() const { return impObj()->elevation(); }
bool PyDbViewportTableRecord::setElevation(double e) { impObj()->setElevation(e); return true; }

// ---- 背景与快速缩放（AcDbViewportTableRecord 自己）----
bool PyDbViewportTableRecord::fastZoomsEnabled() const { return impObj()->fastZoomsEnabled(); }
uint64_t PyDbViewportTableRecord::previousBackground(int type) const {
	AcDbObjectId id = impObj()->previousBackground(static_cast<AcGiDrawable::DrawableType>(type));
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}
bool PyDbViewportTableRecord::previousBackgroundForcedSwitch() const { return impObj()->previousBackgroundForcedSwitch(); }
bool PyDbViewportTableRecord::setBackground(uint64_t backgroundId) {
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(backgroundId));
	return PyWarnIfError(impObj()->setBackground(id), L"AcDbViewportTableRecord::setBackground");
}
bool PyDbViewportTableRecord::setFastZoomsEnabled(bool enabled) { impObj()->setFastZoomsEnabled(enabled); return true; }
bool PyDbViewportTableRecord::setPreviousBackground(uint64_t backgroundId, int type, bool bForcedSwitch) {
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(backgroundId));
	return PyWarnIfError(impObj()->setPreviousBackground(id, static_cast<AcGiDrawable::DrawableType>(type), bForcedSwitch), L"AcDbViewportTableRecord::setPreviousBackground");
}

void bind_PyDbViewportTableRecord(pym& m) {
	py::class_<PyDbViewportTableRecord, PyDbSymbolTableRecord>(m, "PyDbViewportTableRecord")
		.def(py::init<AcDbViewportTableRecord*>(), py::arg("p"))
		PyWrapper_BIND(DbViewportTableRecord)
		.def("number", &PyDbViewportTableRecord::number)
		.def("lowerLeftCorner", &PyDbViewportTableRecord::lowerLeftCorner)
		.def("setLowerLeftCorner", &PyDbViewportTableRecord::setLowerLeftCorner, py::arg("pt"))
		.def("upperRightCorner", &PyDbViewportTableRecord::upperRightCorner)
		.def("setUpperRightCorner", &PyDbViewportTableRecord::setUpperRightCorner, py::arg("pt"))
		.def("ucsFollowMode", &PyDbViewportTableRecord::ucsFollowMode)
		.def("setUcsFollowMode", &PyDbViewportTableRecord::setUcsFollowMode, py::arg("enabled"))
		.def("circleSides", &PyDbViewportTableRecord::circleSides)
		.def("setCircleSides", &PyDbViewportTableRecord::setCircleSides, py::arg("sides"))
		.def("iconEnabled", &PyDbViewportTableRecord::iconEnabled)
		.def("setIconEnabled", &PyDbViewportTableRecord::setIconEnabled, py::arg("enabled"))
		.def("iconAtOrigin", &PyDbViewportTableRecord::iconAtOrigin)
		.def("setIconAtOrigin", &PyDbViewportTableRecord::setIconAtOrigin, py::arg("atOrigin"))
		.def("gridEnabled", &PyDbViewportTableRecord::gridEnabled)
		.def("setGridEnabled", &PyDbViewportTableRecord::setGridEnabled, py::arg("enabled"))
		.def("gridIncrements", &PyDbViewportTableRecord::gridIncrements)
		.def("setGridIncrements", &PyDbViewportTableRecord::setGridIncrements, py::arg("pt"))
		.def("snapEnabled", &PyDbViewportTableRecord::snapEnabled)
		.def("setSnapEnabled", &PyDbViewportTableRecord::setSnapEnabled, py::arg("enabled"))
		.def("isometricSnapEnabled", &PyDbViewportTableRecord::isometricSnapEnabled)
		.def("setIsometricSnapEnabled", &PyDbViewportTableRecord::setIsometricSnapEnabled, py::arg("enabled"))
		.def("snapPair", &PyDbViewportTableRecord::snapPair)
		.def("setSnapPair", &PyDbViewportTableRecord::setSnapPair, py::arg("pairType"))
		.def("snapAngle", &PyDbViewportTableRecord::snapAngle)
		.def("setSnapAngle", &PyDbViewportTableRecord::setSnapAngle, py::arg("angle"))
		.def("snapBase", &PyDbViewportTableRecord::snapBase)
		.def("setSnapBase", &PyDbViewportTableRecord::setSnapBase, py::arg("pt"))
		.def("snapIncrements", &PyDbViewportTableRecord::snapIncrements)
		.def("setSnapIncrements", &PyDbViewportTableRecord::setSnapIncrements, py::arg("pt"))
		.def("isUcsSavedWithViewport", &PyDbViewportTableRecord::isUcsSavedWithViewport)
		.def("setUcsPerViewport", &PyDbViewportTableRecord::setUcsPerViewport, py::arg("ucsvp"))
		.def("isGridBoundToLimits", &PyDbViewportTableRecord::isGridBoundToLimits)
		.def("setGridBoundToLimits", &PyDbViewportTableRecord::setGridBoundToLimits, py::arg("enabled"))
		.def("isGridAdaptive", &PyDbViewportTableRecord::isGridAdaptive)
		.def("setGridAdaptive", &PyDbViewportTableRecord::setGridAdaptive, py::arg("enabled"))
		.def("isGridSubdivisionRestricted", &PyDbViewportTableRecord::isGridSubdivisionRestricted)
		.def("setGridSubdivisionRestricted", &PyDbViewportTableRecord::setGridSubdivisionRestricted, py::arg("enabled"))
		.def("isGridFollow", &PyDbViewportTableRecord::isGridFollow)
		.def("setGridFollow", &PyDbViewportTableRecord::setGridFollow, py::arg("enabled"))
		.def("gridMajor", &PyDbViewportTableRecord::gridMajor)
		.def("setGridMajor", &PyDbViewportTableRecord::setGridMajor, py::arg("value"))
		.def("centerPoint", &PyDbViewportTableRecord::centerPoint)
		.def("setCenterPoint", &PyDbViewportTableRecord::setCenterPoint, py::arg("pt"))
		.def("height", &PyDbViewportTableRecord::height)
		.def("setHeight", &PyDbViewportTableRecord::setHeight, py::arg("h"))
		.def("width", &PyDbViewportTableRecord::width)
		.def("setWidth", &PyDbViewportTableRecord::setWidth, py::arg("w"))
		.def("target", &PyDbViewportTableRecord::target)
		.def("setTarget", &PyDbViewportTableRecord::setTarget, py::arg("t"))
		.def("viewDirection", &PyDbViewportTableRecord::viewDirection)
		.def("setViewDirection", &PyDbViewportTableRecord::setViewDirection, py::arg("v"))
		.def("viewTwist", &PyDbViewportTableRecord::viewTwist)
		.def("setViewTwist", &PyDbViewportTableRecord::setViewTwist, py::arg("angle"))
		.def("lensLength", &PyDbViewportTableRecord::lensLength)
		.def("setLensLength", &PyDbViewportTableRecord::setLensLength, py::arg("length"))
		.def("perspectiveEnabled", &PyDbViewportTableRecord::perspectiveEnabled)
		.def("setPerspectiveEnabled", &PyDbViewportTableRecord::setPerspectiveEnabled, py::arg("enabled"))
		.def("frontClipDistance", &PyDbViewportTableRecord::frontClipDistance)
		.def("setFrontClipDistance", &PyDbViewportTableRecord::setFrontClipDistance, py::arg("d"))
		.def("backClipDistance", &PyDbViewportTableRecord::backClipDistance)
		.def("setBackClipDistance", &PyDbViewportTableRecord::setBackClipDistance, py::arg("d"))
		.def("frontClipEnabled", &PyDbViewportTableRecord::frontClipEnabled)
		.def("setFrontClipEnabled", &PyDbViewportTableRecord::setFrontClipEnabled, py::arg("enabled"))
		.def("backClipEnabled", &PyDbViewportTableRecord::backClipEnabled)
		.def("setBackClipEnabled", &PyDbViewportTableRecord::setBackClipEnabled, py::arg("enabled"))
		.def("elevation", &PyDbViewportTableRecord::elevation)
		.def("setElevation", &PyDbViewportTableRecord::setElevation, py::arg("e"))
		// ---- 背景与快速缩放 ----
		.def("fastZoomsEnabled", &PyDbViewportTableRecord::fastZoomsEnabled)
		.def("previousBackground", &PyDbViewportTableRecord::previousBackground, py::arg("type") = 0, "取上一背景 ObjectId（uint64）；type 为 AcGiDrawable::DrawableType 整型（0=kGeometry），无背景返回 0")
		.def("previousBackgroundForcedSwitch", &PyDbViewportTableRecord::previousBackgroundForcedSwitch)
		.def("setBackground", &PyDbViewportTableRecord::setBackground, py::arg("backgroundId"), "backgroundId 为 uint64 ObjectId（asOldId），0 表示清除")
		.def("setFastZoomsEnabled", &PyDbViewportTableRecord::setFastZoomsEnabled, py::arg("enabled"))
		.def("setPreviousBackground", &PyDbViewportTableRecord::setPreviousBackground, py::arg("backgroundId"), py::arg("type") = 0, py::arg("bForcedSwitch") = false);
}
