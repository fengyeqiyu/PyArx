#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbAlignedDimension.h"

PyWrapper_Cpp(DbAlignedDimension)
AcString PyDbAlignedDimension::toString() const { return AcString(std::format("AcDbAlignedDimension[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbAlignedDimension::xLine1Point() const { return impObj()->xLine1Point(); }

bool  PyDbAlignedDimension::setXLine1Point(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setXLine1Point(p), L"AcDbAlignedDimension::setXLine1Point"); }

AcGePoint3d PyDbAlignedDimension::xLine2Point() const { return impObj()->xLine2Point(); }

bool  PyDbAlignedDimension::setXLine2Point(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setXLine2Point(p), L"AcDbAlignedDimension::setXLine2Point"); }

AcGePoint3d PyDbAlignedDimension::dimLinePoint() const { return impObj()->dimLinePoint(); }

bool  PyDbAlignedDimension::setDimLinePoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setDimLinePoint(p), L"AcDbAlignedDimension::setDimLinePoint"); }

double PyDbAlignedDimension::oblique() const { return impObj()->oblique(); }
bool  PyDbAlignedDimension::setOblique(double v) { return PyWarnIfError(impObj()->setOblique(v), L"AcDbAlignedDimension::setOblique"); }
bool PyDbAlignedDimension::jogSymbolOn() const { return impObj()->jogSymbolOn(); }
bool  PyDbAlignedDimension::setJogSymbolOn(bool v) { return PyWarnIfError(impObj()->setJogSymbolOn(v), L"AcDbAlignedDimension::setJogSymbolOn"); }
AcGePoint3d PyDbAlignedDimension::jogSymbolPosition() const { return impObj()->jogSymbolPosition(); }
bool  PyDbAlignedDimension::setJogSymbolPosition(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setJogSymbolPosition(p), L"AcDbAlignedDimension::setJogSymbolPosition"); }

void bind_PyDbAlignedDimension(pym& m) {
	py::class_<PyDbAlignedDimension, PyDbDimension>(m, "PyDbAlignedDimension")
		.def(py::init<>())
		.def(py::init<AcDbAlignedDimension*>(), py::arg("object"))
		.def("xLine1Point", &PyDbAlignedDimension::xLine1Point)
		.def("setXLine1Point", &PyDbAlignedDimension::setXLine1Point, py::arg("p"))
		.def("xLine2Point", &PyDbAlignedDimension::xLine2Point)
		.def("setXLine2Point", &PyDbAlignedDimension::setXLine2Point, py::arg("p"))
		.def("dimLinePoint", &PyDbAlignedDimension::dimLinePoint)
		.def("setDimLinePoint", &PyDbAlignedDimension::setDimLinePoint, py::arg("p"))
		.def("oblique", &PyDbAlignedDimension::oblique)
		.def("setOblique", &PyDbAlignedDimension::setOblique, py::arg("v"))
		.def("jogSymbolOn", &PyDbAlignedDimension::jogSymbolOn)
		.def("setJogSymbolOn", &PyDbAlignedDimension::setJogSymbolOn, py::arg("v"))
		.def("jogSymbolPosition", &PyDbAlignedDimension::jogSymbolPosition)
		.def("setJogSymbolPosition", &PyDbAlignedDimension::setJogSymbolPosition, py::arg("p"))
		PyWrapper_BIND(DbAlignedDimension)
		.def("__repr__", &PyDbAlignedDimension::toString)
		;
}

