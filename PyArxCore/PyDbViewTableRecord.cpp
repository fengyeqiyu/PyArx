#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbViewTableRecord.h"

PyWrapper_Cpp(DbViewTableRecord);

// ---- AcDbViewTableRecord own ----
bool PyDbViewTableRecord::isPaperspaceView() const { if (!impObj()) return false; return impObj()->isPaperspaceView(); }
bool PyDbViewTableRecord::setIsPaperspaceView(bool pspace) { if (!impObj()) return false; impObj()->setIsPaperspaceView(pspace); return true; }
bool PyDbViewTableRecord::isUcsAssociatedToView() const { if (!impObj()) return false; return impObj()->isUcsAssociatedToView(); }
bool PyDbViewTableRecord::disassociateUcsFromView() { if (!impObj()) return false; return PyWarnIfError(impObj()->disassociateUcsFromView(), L"AcDbViewTableRecord::disassociateUcsFromView"); }

AcString PyDbViewTableRecord::getCategoryName() const {
	if (!impObj()) return AcString();
	AcString s; if(!PyWarnIfError(impObj()->getCategoryName(s), L"AcDbViewTableRecord::getCategoryName")) return AcString(); return s;
}
bool PyDbViewTableRecord::setCategoryName(const AcString& name) { if (!impObj()) return false; return PyWarnIfError(impObj()->setCategoryName(name.kwszPtr()), L"AcDbViewTableRecord::setCategoryName"); }
AcString PyDbViewTableRecord::getLayerState() const {
	if (!impObj()) return AcString();
	AcString s; if(!PyWarnIfError(impObj()->getLayerState(s), L"AcDbViewTableRecord::getLayerState")) return AcString(); return s;
}
bool PyDbViewTableRecord::setLayerState(const AcString& name) { if (!impObj()) return false; return PyWarnIfError(impObj()->setLayerState(name.kwszPtr()), L"AcDbViewTableRecord::setLayerState"); }

uint64_t PyDbViewTableRecord::getLayout() const {
	if (!impObj()) return 0ULL;
	AcDbObjectId id; if(!PyWarnIfError(impObj()->getLayout(id), L"AcDbViewTableRecord::getLayout")) return 0ULL; return static_cast<uint64_t>(id.asOldId());
}
bool PyDbViewTableRecord::setLayout(uint64_t layoutId) {
	if (!impObj()) return false;
	AcDbObjectId id; id.setFromOldId(static_cast<Adesk::IntDbId>(layoutId));
	return PyWarnIfError(impObj()->setLayout(id), L"AcDbViewTableRecord::setLayout");
}
bool PyDbViewTableRecord::isViewAssociatedToViewport() const { if (!impObj()) return false; return impObj()->isViewAssociatedToViewport(); }
bool PyDbViewTableRecord::setViewAssociatedToViewport(bool flag) { if (!impObj()) return false; return PyWarnIfError(impObj()->setViewAssociatedToViewport(flag), L"AcDbViewTableRecord::setViewAssociatedToViewport"); }
bool PyDbViewTableRecord::isCameraPlottable() const { if (!impObj()) return false; return impObj()->isCameraPlottable(); }
bool PyDbViewTableRecord::setIsCameraPlottable(bool plottable) { if (!impObj()) return false; return PyWarnIfError(impObj()->setIsCameraPlottable(plottable), L"AcDbViewTableRecord::setIsCameraPlottable"); }

// ---- 相机 / 活截面 / 太阳 / 注释比例（本类自有）----
uint64_t PyDbViewTableRecord::camera() const {
	if (!impObj()) return 0ULL;
	AcDbObjectId id = impObj()->camera();
	return static_cast<uint64_t>(id.asOldId());
}
uint64_t PyDbViewTableRecord::liveSection() const {
	if (!impObj()) return 0ULL;
	AcDbObjectId id = impObj()->liveSection();
	return static_cast<uint64_t>(id.asOldId());
}
uint64_t PyDbViewTableRecord::sunId() const {
	if (!impObj()) return 0ULL;
	AcDbObjectId id = impObj()->sunId();
	return static_cast<uint64_t>(id.asOldId());
}
bool PyDbViewTableRecord::setCamera(uint64_t cameraId) {
	if (!impObj()) return false;
	AcDbObjectId id; id.setFromOldId(static_cast<Adesk::IntDbId>(cameraId));
	return PyWarnIfError(impObj()->setCamera(id), L"AcDbViewTableRecord::setCamera");
}
bool PyDbViewTableRecord::setLiveSection(uint64_t liveSectionId) {
	if (!impObj()) return false;
	AcDbObjectId id; id.setFromOldId(static_cast<Adesk::IntDbId>(liveSectionId));
	return PyWarnIfError(impObj()->setLiveSection(id), L"AcDbViewTableRecord::setLiveSection");
}
bool PyDbViewTableRecord::setParametersFromViewport(uint64_t viewportId) {
	if (!impObj()) return false;
	AcDbObjectId id; id.setFromOldId(static_cast<Adesk::IntDbId>(viewportId));
	return PyWarnIfError(impObj()->setParametersFromViewport(id), L"AcDbViewTableRecord::setParametersFromViewport");
}
bool PyDbViewTableRecord::setAnnotationScale(PyDbAnnotationScale& scale) {
	if (!impObj()) return false;
	if (scale.isNull()) return false;
	return PyWarnIfError(impObj()->setAnnotationScale(scale.impObj()), L"AcDbViewTableRecord::setAnnotationScale");
}
std::tuple<bool, uint64_t> PyDbViewTableRecord::setSun(PyDbObject& sunObj) {
	return setSun(sunObj, true);
}
std::tuple<bool, uint64_t> PyDbViewTableRecord::setSun(PyDbObject& sunObj, bool eraseOldSun) {
	if (!impObj()) return {};
	AcDbObject* pSun = sunObj.impObj();
	if (!pSun) return std::make_tuple(false, 0ULL);
	AcDbObjectId retId;
	bool ok = PyWarnIfError(impObj()->setSun(retId, pSun, eraseOldSun), L"AcDbViewTableRecord::setSun");
	return std::make_tuple(ok, ok ? static_cast<uint64_t>(retId.asOldId()) : 0ULL);
}

// ---- Inherited from AcDbAbstractViewTableRecord ----
AcGePoint2d PyDbViewTableRecord::centerPoint() const { if (!impObj()) return AcGePoint2d(); return impObj()->centerPoint(); }
bool PyDbViewTableRecord::setCenterPoint(const AcGePoint2d& pt) { if (!impObj()) return false; impObj()->setCenterPoint(pt); return true; }
double PyDbViewTableRecord::height() const { if (!impObj()) return 0.0; return impObj()->height(); }
bool PyDbViewTableRecord::setHeight(double h) { if (!impObj()) return false; impObj()->setHeight(h); return true; }
double PyDbViewTableRecord::width() const { if (!impObj()) return 0.0; return impObj()->width(); }
bool PyDbViewTableRecord::setWidth(double w) { if (!impObj()) return false; impObj()->setWidth(w); return true; }
AcGePoint3d PyDbViewTableRecord::target() const { if (!impObj()) return AcGePoint3d(); return impObj()->target(); }
bool PyDbViewTableRecord::setTarget(const AcGePoint3d& t) { if (!impObj()) return false; impObj()->setTarget(t); return true; }
AcGeVector3d PyDbViewTableRecord::viewDirection() const { if (!impObj()) return AcGeVector3d(); return impObj()->viewDirection(); }
bool PyDbViewTableRecord::setViewDirection(const AcGeVector3d& v) { if (!impObj()) return false; impObj()->setViewDirection(v); return true; }
double PyDbViewTableRecord::viewTwist() const { if (!impObj()) return 0.0; return impObj()->viewTwist(); }
bool PyDbViewTableRecord::setViewTwist(double angle) { if (!impObj()) return false; impObj()->setViewTwist(angle); return true; }
double PyDbViewTableRecord::lensLength() const { if (!impObj()) return 0.0; return impObj()->lensLength(); }
bool PyDbViewTableRecord::setLensLength(double length) { if (!impObj()) return false; impObj()->setLensLength(length); return true; }
bool PyDbViewTableRecord::perspectiveEnabled() const { if (!impObj()) return false; return impObj()->perspectiveEnabled(); }
bool PyDbViewTableRecord::setPerspectiveEnabled(bool enabled) { if (!impObj()) return false; impObj()->setPerspectiveEnabled(enabled); return true; }

// ---- 裁剪平面与高程（继承自 AcDbAbstractViewTableRecord）----
double PyDbViewTableRecord::frontClipDistance() const { if (!impObj()) return 0.0; return impObj()->frontClipDistance(); }
bool PyDbViewTableRecord::setFrontClipDistance(double d) { if (!impObj()) return false; impObj()->setFrontClipDistance(d); return true; }
double PyDbViewTableRecord::backClipDistance() const { if (!impObj()) return 0.0; return impObj()->backClipDistance(); }
bool PyDbViewTableRecord::setBackClipDistance(double d) { if (!impObj()) return false; impObj()->setBackClipDistance(d); return true; }
bool PyDbViewTableRecord::frontClipEnabled() const { if (!impObj()) return false; return impObj()->frontClipEnabled(); }
bool PyDbViewTableRecord::setFrontClipEnabled(bool enabled) { if (!impObj()) return false; impObj()->setFrontClipEnabled(enabled); return true; }
bool PyDbViewTableRecord::backClipEnabled() const { if (!impObj()) return false; return impObj()->backClipEnabled(); }
bool PyDbViewTableRecord::setBackClipEnabled(bool enabled) { if (!impObj()) return false; impObj()->setBackClipEnabled(enabled); return true; }
double PyDbViewTableRecord::elevation() const { if (!impObj()) return 0.0; return impObj()->elevation(); }
bool PyDbViewTableRecord::setElevation(double e) { if (!impObj()) return false; impObj()->setElevation(e); return true; }

void bind_PyDbViewTableRecord(pym& m) {
	py::class_<PyDbViewTableRecord, PyDbSymbolTableRecord>(m, "PyDbViewTableRecord")
		.def(py::init<AcDbViewTableRecord*>(), py::arg("p"))
		PyWrapper_BIND(DbViewTableRecord)
		.def("isPaperspaceView", &PyDbViewTableRecord::isPaperspaceView)
		.def("setIsPaperspaceView", &PyDbViewTableRecord::setIsPaperspaceView, py::arg("pspace"))
		.def("isUcsAssociatedToView", &PyDbViewTableRecord::isUcsAssociatedToView)
		.def("disassociateUcsFromView", &PyDbViewTableRecord::disassociateUcsFromView)
		.def("getCategoryName", &PyDbViewTableRecord::getCategoryName)
		.def("setCategoryName", &PyDbViewTableRecord::setCategoryName, py::arg("name"))
		.def("getLayerState", &PyDbViewTableRecord::getLayerState)
		.def("setLayerState", &PyDbViewTableRecord::setLayerState, py::arg("name"))
		.def("getLayout", &PyDbViewTableRecord::getLayout)
		.def("setLayout", &PyDbViewTableRecord::setLayout, py::arg("layoutId"))
		.def("isViewAssociatedToViewport", &PyDbViewTableRecord::isViewAssociatedToViewport)
		.def("setViewAssociatedToViewport", &PyDbViewTableRecord::setViewAssociatedToViewport, py::arg("flag"))
		.def("isCameraPlottable", &PyDbViewTableRecord::isCameraPlottable)
		.def("setIsCameraPlottable", &PyDbViewTableRecord::setIsCameraPlottable, py::arg("plottable"))
		.def("camera", &PyDbViewTableRecord::camera)
		.def("liveSection", &PyDbViewTableRecord::liveSection)
		.def("sunId", &PyDbViewTableRecord::sunId)
		.def("setCamera", &PyDbViewTableRecord::setCamera, py::arg("cameraId"))
		.def("setLiveSection", &PyDbViewTableRecord::setLiveSection, py::arg("liveSectionId"))
		.def("setParametersFromViewport", &PyDbViewTableRecord::setParametersFromViewport, py::arg("viewportId"))
		.def("setAnnotationScale", &PyDbViewTableRecord::setAnnotationScale, py::arg("scale"))
		.def("setSun", [](PyDbViewTableRecord& self, PyDbObject& sunObj) { return self.setSun(sunObj); }, py::arg("sunObj"))
		.def("setSun", [](PyDbViewTableRecord& self, PyDbObject& sunObj, bool eraseOldSun) { return self.setSun(sunObj, eraseOldSun); }, py::arg("sunObj"), py::arg("eraseOldSun"))
		.def("centerPoint", &PyDbViewTableRecord::centerPoint)
		.def("setCenterPoint", &PyDbViewTableRecord::setCenterPoint, py::arg("pt"))
		.def("height", &PyDbViewTableRecord::height)
		.def("setHeight", &PyDbViewTableRecord::setHeight, py::arg("h"))
		.def("width", &PyDbViewTableRecord::width)
		.def("setWidth", &PyDbViewTableRecord::setWidth, py::arg("w"))
		.def("target", &PyDbViewTableRecord::target)
		.def("setTarget", &PyDbViewTableRecord::setTarget, py::arg("t"))
		.def("viewDirection", &PyDbViewTableRecord::viewDirection)
		.def("setViewDirection", &PyDbViewTableRecord::setViewDirection, py::arg("v"))
		.def("viewTwist", &PyDbViewTableRecord::viewTwist)
		.def("setViewTwist", &PyDbViewTableRecord::setViewTwist, py::arg("angle"))
		.def("lensLength", &PyDbViewTableRecord::lensLength)
		.def("setLensLength", &PyDbViewTableRecord::setLensLength, py::arg("length"))
		.def("perspectiveEnabled", &PyDbViewTableRecord::perspectiveEnabled)
		.def("setPerspectiveEnabled", &PyDbViewTableRecord::setPerspectiveEnabled, py::arg("enabled"))
		.def("frontClipDistance", &PyDbViewTableRecord::frontClipDistance)
		.def("setFrontClipDistance", &PyDbViewTableRecord::setFrontClipDistance, py::arg("d"))
		.def("backClipDistance", &PyDbViewTableRecord::backClipDistance)
		.def("setBackClipDistance", &PyDbViewTableRecord::setBackClipDistance, py::arg("d"))
		.def("frontClipEnabled", &PyDbViewTableRecord::frontClipEnabled)
		.def("setFrontClipEnabled", &PyDbViewTableRecord::setFrontClipEnabled, py::arg("enabled"))
		.def("backClipEnabled", &PyDbViewTableRecord::backClipEnabled)
		.def("setBackClipEnabled", &PyDbViewTableRecord::setBackClipEnabled, py::arg("enabled"))
		.def("elevation", &PyDbViewTableRecord::elevation)
		.def("setElevation", &PyDbViewTableRecord::setElevation, py::arg("e"));
}
