#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbWipeout.h"

PyWrapper_Cpp(DbWipeout)
AcString PyDbWipeout::toString() const { return AcString(std::format("AcDbWipeout[wrapper]").c_str(), AcString::Utf8); }
bool PyDbWipeout::frame() const { return impObj()->frame() == Adesk::kTrue; }

bool PyDbWipeout::isClipped() const { return impObj()->isClipped() != Adesk::kFalse; }
int PyDbWipeout::brightness() const { return static_cast<int>(impObj()->brightness()); }
bool  PyDbWipeout::setBrightness(int v) { return PyWarnIfError(impObj()->setBrightness(static_cast<Adesk::Int8>(v)), L"AcDbWipeout::setBrightness"); }
int PyDbWipeout::contrast() const { return static_cast<int>(impObj()->contrast()); }
bool  PyDbWipeout::setContrast(int v) { return PyWarnIfError(impObj()->setContrast(static_cast<Adesk::Int8>(v)), L"AcDbWipeout::setContrast"); }
int PyDbWipeout::fade() const { return static_cast<int>(impObj()->fade()); }
bool  PyDbWipeout::setFade(int v) { return PyWarnIfError(impObj()->setFade(static_cast<Adesk::Int8>(v)), L"AcDbWipeout::setFade"); }
AcGeVector2d PyDbWipeout::scale() const { return impObj()->scale(); }
AcGeVector2d PyDbWipeout::imageSize(bool cached) const { return impObj()->imageSize(cached ? Adesk::kTrue : Adesk::kFalse); }
bool PyDbWipeout::append(uint64_t id) {
	AcDbObjectId oid;
	oid.setFromOldId(static_cast<Adesk::UIntPtr>(id));
	return PyWarnIfError(impObj()->append(oid), L"AcDbWipeout::append");
}

void bind_PyDbWipeout(pym& m) {
	py::class_<PyDbWipeout, PyDbEntity>(m, "PyDbWipeout")
		.def(py::init<>())
		.def(py::init<AcDbWipeout*>(), py::arg("object"))
		.def("frame", &PyDbWipeout::frame)
		.def("isClipped", &PyDbWipeout::isClipped)
		.def("brightness", &PyDbWipeout::brightness)
		.def("setBrightness", &PyDbWipeout::setBrightness, py::arg("value"))
		.def("contrast", &PyDbWipeout::contrast)
		.def("setContrast", &PyDbWipeout::setContrast, py::arg("value"))
		.def("fade", &PyDbWipeout::fade)
		.def("setFade", &PyDbWipeout::setFade, py::arg("value"))
		.def("scale", &PyDbWipeout::scale)
		.def("imageSize", &PyDbWipeout::imageSize, py::arg("cached") = false)
		.def("append", &PyDbWipeout::append, py::arg("id"), "追加裁剪边界对象 id (uint64 oldId)")
		PyWrapper_BIND(DbWipeout)
		.def("__repr__", &PyDbWipeout::toString)
		;
}

