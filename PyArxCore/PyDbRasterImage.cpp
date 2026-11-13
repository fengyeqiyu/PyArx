#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbRasterImage.h"

PyWrapper_Cpp(DbRasterImage)
AcString PyDbRasterImage::toString() const { return AcString(std::format("AcDbRasterImage[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbRasterImage::position() const { return impObj()->position(); }

double PyDbRasterImage::rotation() const { return impObj()->rotation(); }

bool  PyDbRasterImage::setRotation(double r) { return PyWarnIfError(impObj()->setRotation(r), L"AcDbRasterImage::setRotation"); }

uint64_t PyDbRasterImage::imageDefId() const { return static_cast<uint64_t>(impObj()->imageDefId().asOldId()); }

AcGeVector2d PyDbRasterImage::imageSize() const { return impObj()->imageSize(); }

AcGeVector2d PyDbRasterImage::scale() const { return impObj()->scale(); }
double PyDbRasterImage::width() const { return impObj()->width(); }
double PyDbRasterImage::height() const { return impObj()->height(); }
double PyDbRasterImage::imageWidth() const { return impObj()->imageWidth(); }
double PyDbRasterImage::imageHeight() const { return impObj()->imageHeight(); }
bool  PyDbRasterImage::setWidth(double v) { return PyWarnIfError(impObj()->setWidth(v), L"AcDbRasterImage::setWidth"); }
bool  PyDbRasterImage::setHeight(double v) { return PyWarnIfError(impObj()->setHeight(v), L"AcDbRasterImage::setHeight"); }
int PyDbRasterImage::brightness() const { return static_cast<int>(impObj()->brightness()); }
bool  PyDbRasterImage::setBrightness(int v) { return PyWarnIfError(impObj()->setBrightness(static_cast<Adesk::Int8>(v)), L"AcDbRasterImage::setBrightness"); }
int PyDbRasterImage::contrast() const { return static_cast<int>(impObj()->contrast()); }
bool  PyDbRasterImage::setContrast(int v) { return PyWarnIfError(impObj()->setContrast(static_cast<Adesk::Int8>(v)), L"AcDbRasterImage::setContrast"); }
int PyDbRasterImage::fade() const { return static_cast<int>(impObj()->fade()); }
bool  PyDbRasterImage::setFade(int v) { return PyWarnIfError(impObj()->setFade(static_cast<Adesk::Int8>(v)), L"AcDbRasterImage::setFade"); }
bool PyDbRasterImage::isClipped() const { return impObj()->isClipped() != Adesk::kFalse; }
bool PyDbRasterImage::isClipInverted() const { return impObj()->isClipInverted() != Adesk::kFalse; }
void PyDbRasterImage::setClipInverted(bool v) { impObj()->setClipInverted(v ? Adesk::kTrue : Adesk::kFalse); }
bool PyDbRasterImage::isImageShown() const { return impObj()->isImageShown(); }
void PyDbRasterImage::setShowImage(bool v) { impObj()->setShowImage(v); }
bool PyDbRasterImage::isImageTransparent() const { return impObj()->isImageTransparent(); }
void PyDbRasterImage::setImageTransparency(bool v) { impObj()->setImageTransparency(v); }
bool PyDbRasterImage::isShownClipped() const { return impObj()->isShownClipped(); }
void PyDbRasterImage::setShowClipped(bool v) { impObj()->setShowClipped(v); }
bool PyDbRasterImage::castShadows() const { return impObj()->castShadows(); }
void PyDbRasterImage::setCastShadows(bool v) { impObj()->setCastShadows(v); }
bool PyDbRasterImage::receiveShadows() const { return impObj()->receiveShadows(); }
void PyDbRasterImage::setReceiveShadows(bool v) { impObj()->setReceiveShadows(v); }
int PyDbRasterImage::clipBoundaryType() const { return static_cast<int>(impObj()->clipBoundaryType()); }

void bind_PyDbRasterImage(pym& m) {
	py::class_<PyDbRasterImage, PyDbEntity>(m, "PyDbRasterImage")
		.def(py::init<>())
		.def(py::init<AcDbRasterImage*>(), py::arg("object"))
		.def("position", &PyDbRasterImage::position)
		.def("rotation", &PyDbRasterImage::rotation)
		.def("setRotation", &PyDbRasterImage::setRotation, py::arg("r"))
		.def("imageDefId", &PyDbRasterImage::imageDefId)
		.def("imageSize", &PyDbRasterImage::imageSize)
		.def("scale", &PyDbRasterImage::scale)
		.def("width", &PyDbRasterImage::width)
		.def("height", &PyDbRasterImage::height)
		.def("imageWidth", &PyDbRasterImage::imageWidth)
		.def("imageHeight", &PyDbRasterImage::imageHeight)
		.def("setWidth", &PyDbRasterImage::setWidth, py::arg("v"))
		.def("setHeight", &PyDbRasterImage::setHeight, py::arg("v"))
		.def("brightness", &PyDbRasterImage::brightness)
		.def("setBrightness", &PyDbRasterImage::setBrightness, py::arg("v"))
		.def("contrast", &PyDbRasterImage::contrast)
		.def("setContrast", &PyDbRasterImage::setContrast, py::arg("v"))
		.def("fade", &PyDbRasterImage::fade)
		.def("setFade", &PyDbRasterImage::setFade, py::arg("v"))
		.def("isClipped", &PyDbRasterImage::isClipped)
		.def("isClipInverted", &PyDbRasterImage::isClipInverted)
		.def("setClipInverted", &PyDbRasterImage::setClipInverted, py::arg("v"))
		.def("isImageShown", &PyDbRasterImage::isImageShown)
		.def("setShowImage", &PyDbRasterImage::setShowImage, py::arg("v"))
		.def("isImageTransparent", &PyDbRasterImage::isImageTransparent)
		.def("setImageTransparency", &PyDbRasterImage::setImageTransparency, py::arg("v"))
		.def("isShownClipped", &PyDbRasterImage::isShownClipped)
		.def("setShowClipped", &PyDbRasterImage::setShowClipped, py::arg("v"))
		.def("castShadows", &PyDbRasterImage::castShadows)
		.def("setCastShadows", &PyDbRasterImage::setCastShadows, py::arg("v"))
		.def("receiveShadows", &PyDbRasterImage::receiveShadows)
		.def("setReceiveShadows", &PyDbRasterImage::setReceiveShadows, py::arg("v"))
		.def("clipBoundaryType", &PyDbRasterImage::clipBoundaryType)
		PyWrapper_BIND(DbRasterImage)
		.def("__repr__", &PyDbRasterImage::toString)
		;
}

