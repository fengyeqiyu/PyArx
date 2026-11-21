#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbViewportTableRecord.h"

PyWrapper_Cpp(DbViewportTableRecord);

// ---- AcDbViewportTableRecord own ----
int PyDbViewportTableRecord::number() const { if (!impObj()) return 0; return static_cast<int>(impObj()->number()); }
AcGePoint2d PyDbViewportTableRecord::lowerLeftCorner() const { if (!impObj()) return AcGePoint2d(); return impObj()->lowerLeftCorner(); }
bool PyDbViewportTableRecord::setLowerLeftCorner(const AcGePoint2d& pt) { if (!impObj()) return false; impObj()->setLowerLeftCorner(pt); return true; }
AcGePoint2d PyDbViewportTableRecord::upperRightCorner() const { if (!impObj()) return AcGePoint2d(); return impObj()->upperRightCorner(); }
bool PyDbViewportTableRecord::setUpperRightCorner(const AcGePoint2d& pt) { if (!impObj()) return false; impObj()->setUpperRightCorner(pt); return true; }
bool PyDbViewportTableRecord::ucsFollowMode() const { if (!impObj()) return false; return impObj()->ucsFollowMode(); }
bool PyDbViewportTableRecord::setUcsFollowMode(bool enabled) { if (!impObj()) return false; impObj()->setUcsFollowMode(enabled); return true; }
int PyDbViewportTableRecord::circleSides() const { if (!impObj()) return 0; return static_cast<int>(impObj()->circleSides()); }
bool PyDbViewportTableRecord::setCircleSides(int sides) { if (!impObj()) return false; impObj()->setCircleSides(static_cast<Adesk::UInt16>(sides)); return true; }
bool PyDbViewportTableRecord::iconEnabled() const { if (!impObj()) return false; return impObj()->iconEnabled(); }
bool PyDbViewportTableRecord::setIconEnabled(bool enabled) { if (!impObj()) return false; impObj()->setIconEnabled(enabled); return true; }
bool PyDbViewportTableRecord::iconAtOrigin() const { if (!impObj()) return false; return impObj()->iconAtOrigin(); }
bool PyDbViewportTableRecord::setIconAtOrigin(bool atOrigin) { if (!impObj()) return false; impObj()->setIconAtOrigin(atOrigin); return true; }
bool PyDbViewportTableRecord::gridEnabled() const { if (!impObj()) return false; return impObj()->gridEnabled(); }
bool PyDbViewportTableRecord::setGridEnabled(bool enabled) { if (!impObj()) return false; impObj()->setGridEnabled(enabled); return true; }
AcGePoint2d PyDbViewportTableRecord::gridIncrements() const { if (!impObj()) return AcGePoint2d(); return impObj()->gridIncrements(); }
bool PyDbViewportTableRecord::setGridIncrements(const AcGePoint2d& pt) { if (!impObj()) return false; impObj()->setGridIncrements(pt); return true; }
bool PyDbViewportTableRecord::snapEnabled() const { if (!impObj()) return false; return impObj()->snapEnabled(); }
bool PyDbViewportTableRecord::setSnapEnabled(bool enabled) { if (!impObj()) return false; impObj()->setSnapEnabled(enabled); return true; }
bool PyDbViewportTableRecord::isometricSnapEnabled() const { if (!impObj()) return false; return impObj()->isometricSnapEnabled(); }
bool PyDbViewportTableRecord::setIsometricSnapEnabled(bool enabled) { if (!impObj()) return false; impObj()->setIsometricSnapEnabled(enabled); return true; }
int PyDbViewportTableRecord::snapPair() const { if (!impObj()) return 0; return static_cast<int>(impObj()->snapPair()); }
bool PyDbViewportTableRecord::setSnapPair(int pairType) { if (!impObj()) return false; impObj()->setSnapPair(static_cast<Adesk::Int16>(pairType)); return true; }
double PyDbViewportTableRecord::snapAngle() const { if (!impObj()) return 0.0; return impObj()->snapAngle(); }
bool PyDbViewportTableRecord::setSnapAngle(double angle) { if (!impObj()) return false; impObj()->setSnapAngle(angle); return true; }
AcGePoint2d PyDbViewportTableRecord::snapBase() const { if (!impObj()) return AcGePoint2d(); return impObj()->snapBase(); }
bool PyDbViewportTableRecord::setSnapBase(const AcGePoint2d& pt) { if (!impObj()) return false; impObj()->setSnapBase(pt); return true; }
AcGePoint2d PyDbViewportTableRecord::snapIncrements() const { if (!impObj()) return AcGePoint2d(); return impObj()->snapIncrements(); }
bool PyDbViewportTableRecord::setSnapIncrements(const AcGePoint2d& pt) { if (!impObj()) return false; impObj()->setSnapIncrements(pt); return true; }
bool PyDbViewportTableRecord::isUcsSavedWithViewport() const { if (!impObj()) return false; return impObj()->isUcsSavedWithViewport(); }
bool PyDbViewportTableRecord::setUcsPerViewport(bool ucsvp) { if (!impObj()) return false; impObj()->setUcsPerViewport(ucsvp); return true; }
bool PyDbViewportTableRecord::isGridBoundToLimits() const { if (!impObj()) return false; return impObj()->isGridBoundToLimits(); }
bool PyDbViewportTableRecord::setGridBoundToLimits(bool enabled) { if (!impObj()) return false; impObj()->setGridBoundToLimits(enabled); return true; }
bool PyDbViewportTableRecord::isGridAdaptive() const { if (!impObj()) return false; return impObj()->isGridAdaptive(); }
bool PyDbViewportTableRecord::setGridAdaptive(bool enabled) { if (!impObj()) return false; impObj()->setGridAdaptive(enabled); return true; }
bool PyDbViewportTableRecord::isGridSubdivisionRestricted() const { if (!impObj()) return false; return impObj()->isGridSubdivisionRestricted(); }
bool PyDbViewportTableRecord::setGridSubdivisionRestricted(bool enabled) { if (!impObj()) return false; impObj()->setGridSubdivisionRestricted(enabled); return true; }
bool PyDbViewportTableRecord::isGridFollow() const { if (!impObj()) return false; return impObj()->isGridFollow(); }
bool PyDbViewportTableRecord::setGridFollow(bool enabled) { if (!impObj()) return false; impObj()->setGridFollow(enabled); return true; }
int PyDbViewportTableRecord::gridMajor() const { if (!impObj()) return 0; return static_cast<int>(impObj()->gridMajor()); }
bool PyDbViewportTableRecord::setGridMajor(int value) { if (!impObj()) return false; impObj()->setGridMajor(static_cast<Adesk::Int16>(value)); return true; }

// ---- Inherited from AcDbAbstractViewTableRecord ----
AcGePoint2d PyDbViewportTableRecord::centerPoint() const { if (!impObj()) return AcGePoint2d(); return impObj()->centerPoint(); }
bool PyDbViewportTableRecord::setCenterPoint(const AcGePoint2d& pt) { if (!impObj()) return false; impObj()->setCenterPoint(pt); return true; }
double PyDbViewportTableRecord::height() const { if (!impObj()) return 0.0; return impObj()->height(); }
bool PyDbViewportTableRecord::setHeight(double h) { if (!impObj()) return false; impObj()->setHeight(h); return true; }
double PyDbViewportTableRecord::width() const { if (!impObj()) return 0.0; return impObj()->width(); }
bool PyDbViewportTableRecord::setWidth(double w) { if (!impObj()) return false; impObj()->setWidth(w); return true; }
AcGePoint3d PyDbViewportTableRecord::target() const { if (!impObj()) return AcGePoint3d(); return impObj()->target(); }
bool PyDbViewportTableRecord::setTarget(const AcGePoint3d& t) { if (!impObj()) return false; impObj()->setTarget(t); return true; }
AcGeVector3d PyDbViewportTableRecord::viewDirection() const { if (!impObj()) return AcGeVector3d(); return impObj()->viewDirection(); }
bool PyDbViewportTableRecord::setViewDirection(const AcGeVector3d& v) { if (!impObj()) return false; impObj()->setViewDirection(v); return true; }
double PyDbViewportTableRecord::viewTwist() const { if (!impObj()) return 0.0; return impObj()->viewTwist(); }
bool PyDbViewportTableRecord::setViewTwist(double angle) { if (!impObj()) return false; impObj()->setViewTwist(angle); return true; }
double PyDbViewportTableRecord::lensLength() const { if (!impObj()) return 0.0; return impObj()->lensLength(); }
bool PyDbViewportTableRecord::setLensLength(double length) { if (!impObj()) return false; impObj()->setLensLength(length); return true; }
bool PyDbViewportTableRecord::perspectiveEnabled() const { if (!impObj()) return false; return impObj()->perspectiveEnabled(); }
bool PyDbViewportTableRecord::setPerspectiveEnabled(bool enabled) { if (!impObj()) return false; impObj()->setPerspectiveEnabled(enabled); return true; }

// ---- 裁剪平面与高程（继承自 AcDbAbstractViewTableRecord）----
double PyDbViewportTableRecord::frontClipDistance() const { if (!impObj()) return 0.0; return impObj()->frontClipDistance(); }
bool PyDbViewportTableRecord::setFrontClipDistance(double d) { if (!impObj()) return false; impObj()->setFrontClipDistance(d); return true; }
double PyDbViewportTableRecord::backClipDistance() const { if (!impObj()) return 0.0; return impObj()->backClipDistance(); }
bool PyDbViewportTableRecord::setBackClipDistance(double d) { if (!impObj()) return false; impObj()->setBackClipDistance(d); return true; }
bool PyDbViewportTableRecord::frontClipEnabled() const { if (!impObj()) return false; return impObj()->frontClipEnabled(); }
bool PyDbViewportTableRecord::setFrontClipEnabled(bool enabled) { if (!impObj()) return false; impObj()->setFrontClipEnabled(enabled); return true; }
bool PyDbViewportTableRecord::backClipEnabled() const { if (!impObj()) return false; return impObj()->backClipEnabled(); }
bool PyDbViewportTableRecord::setBackClipEnabled(bool enabled) { if (!impObj()) return false; impObj()->setBackClipEnabled(enabled); return true; }
double PyDbViewportTableRecord::elevation() const { if (!impObj()) return 0.0; return impObj()->elevation(); }
bool PyDbViewportTableRecord::setElevation(double e) { if (!impObj()) return false; return PyWarnIfError(impObj()->setElevation(e), L"AcDbAbstractViewTableRecord::setElevation"); }

// ---- 背景与快速缩放（AcDbViewportTableRecord 自己）----
bool PyDbViewportTableRecord::fastZoomsEnabled() const { if (!impObj()) return false; return impObj()->fastZoomsEnabled(); }
uint64_t PyDbViewportTableRecord::previousBackground(int type) const {
	if (!impObj()) return 0ULL;
	AcDbObjectId id = impObj()->previousBackground(static_cast<AcGiDrawable::DrawableType>(type));
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}
bool PyDbViewportTableRecord::previousBackgroundForcedSwitch() const { if (!impObj()) return false; return impObj()->previousBackgroundForcedSwitch(); }
bool PyDbViewportTableRecord::setBackground(uint64_t backgroundId) {
	if (!impObj()) return false;
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(backgroundId));
	return PyWarnIfError(impObj()->setBackground(id), L"AcDbViewportTableRecord::setBackground");
}
bool PyDbViewportTableRecord::setFastZoomsEnabled(bool enabled) { if (!impObj()) return false; impObj()->setFastZoomsEnabled(enabled); return true; }
bool PyDbViewportTableRecord::setPreviousBackground(uint64_t backgroundId, int type, bool bForcedSwitch) {
	if (!impObj()) return false;
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(backgroundId));
	return PyWarnIfError(impObj()->setPreviousBackground(id, static_cast<AcGiDrawable::DrawableType>(type), bForcedSwitch), L"AcDbViewportTableRecord::setPreviousBackground");
}

void bind_PyDbViewportTableRecord(pym& m) {
	py::class_<PyDbViewportTableRecord, PyDbSymbolTableRecord>(m, "PyDbViewportTableRecord")
		.def(py::init<>())
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
