#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbRotatedDimension.h"

PyWrapper_Cpp(DbRotatedDimension)
AcString PyDbRotatedDimension::toString() const { return AcString(std::format("AcDbRotatedDimension[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbRotatedDimension::dimLinePoint() const { return impObj()->dimLinePoint(); }

bool  PyDbRotatedDimension::setDimLinePoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setDimLinePoint(p), L"AcDbRotatedDimension::setDimLinePoint"); }

double PyDbRotatedDimension::rotation() const { return impObj()->rotation(); }

bool  PyDbRotatedDimension::setRotation(double r) { return PyWarnIfError(impObj()->setRotation(r), L"AcDbRotatedDimension::setRotation"); }

AcGePoint3d PyDbRotatedDimension::xLine1Point() const { return impObj()->xLine1Point(); }
bool  PyDbRotatedDimension::setXLine1Point(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setXLine1Point(p), L"AcDbRotatedDimension::setXLine1Point"); }
AcGePoint3d PyDbRotatedDimension::xLine2Point() const { return impObj()->xLine2Point(); }
bool  PyDbRotatedDimension::setXLine2Point(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setXLine2Point(p), L"AcDbRotatedDimension::setXLine2Point"); }
double PyDbRotatedDimension::oblique() const { return impObj()->oblique(); }
bool  PyDbRotatedDimension::setOblique(double v) { return PyWarnIfError(impObj()->setOblique(v), L"AcDbRotatedDimension::setOblique"); }
bool PyDbRotatedDimension::jogSymbolOn() const { return impObj()->jogSymbolOn(); }
bool  PyDbRotatedDimension::setJogSymbolOn(bool v) { return PyWarnIfError(impObj()->setJogSymbolOn(v), L"AcDbRotatedDimension::setJogSymbolOn"); }
AcGePoint3d PyDbRotatedDimension::jogSymbolPosition() const { return impObj()->jogSymbolPosition(); }
bool  PyDbRotatedDimension::setJogSymbolPosition(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setJogSymbolPosition(p), L"AcDbRotatedDimension::setJogSymbolPosition"); }

void bind_PyDbRotatedDimension(pym& m) {
	py::class_<PyDbRotatedDimension, PyDbDimension>(m, "PyDbRotatedDimension")
		.def(py::init<>())
		.def(py::init<AcDbRotatedDimension*>(), py::arg("object"))
		.def("xLine1Point", &PyDbRotatedDimension::xLine1Point)
		.def("setXLine1Point", &PyDbRotatedDimension::setXLine1Point, py::arg("p"))
		.def("xLine2Point", &PyDbRotatedDimension::xLine2Point)
		.def("setXLine2Point", &PyDbRotatedDimension::setXLine2Point, py::arg("p"))
		.def("dimLinePoint", &PyDbRotatedDimension::dimLinePoint)
		.def("setDimLinePoint", &PyDbRotatedDimension::setDimLinePoint, py::arg("p"))
		.def("oblique", &PyDbRotatedDimension::oblique)
		.def("setOblique", &PyDbRotatedDimension::setOblique, py::arg("v"))
		.def("rotation", &PyDbRotatedDimension::rotation)
		.def("setRotation", &PyDbRotatedDimension::setRotation, py::arg("r"))
		.def("jogSymbolOn", &PyDbRotatedDimension::jogSymbolOn)
		.def("setJogSymbolOn", &PyDbRotatedDimension::setJogSymbolOn, py::arg("v"))
		.def("jogSymbolPosition", &PyDbRotatedDimension::jogSymbolPosition)
		.def("setJogSymbolPosition", &PyDbRotatedDimension::setJogSymbolPosition, py::arg("p"))
		PyWrapper_BIND(DbRotatedDimension)
		.def("__repr__", &PyDbRotatedDimension::toString)
		;
}

