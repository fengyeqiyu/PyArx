#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbAlignedDimension.h"

PyWrapper_Cpp(DbAlignedDimension)
AcString PyDbAlignedDimension::toString() const { return AcString(std::format("AcDbAlignedDimension[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbAlignedDimension::xLine1Point() const { if (!impObj()) return AcGePoint3d(); return impObj()->xLine1Point(); }

bool  PyDbAlignedDimension::setXLine1Point(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setXLine1Point(p), L"AcDbAlignedDimension::setXLine1Point"); }

AcGePoint3d PyDbAlignedDimension::xLine2Point() const { if (!impObj()) return AcGePoint3d(); return impObj()->xLine2Point(); }

bool  PyDbAlignedDimension::setXLine2Point(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setXLine2Point(p), L"AcDbAlignedDimension::setXLine2Point"); }

AcGePoint3d PyDbAlignedDimension::dimLinePoint() const { if (!impObj()) return AcGePoint3d(); return impObj()->dimLinePoint(); }

bool  PyDbAlignedDimension::setDimLinePoint(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDimLinePoint(p), L"AcDbAlignedDimension::setDimLinePoint"); }

double PyDbAlignedDimension::oblique() const { if (!impObj()) return 0.0; return impObj()->oblique(); }
bool  PyDbAlignedDimension::setOblique(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setOblique(v), L"AcDbAlignedDimension::setOblique"); }
bool PyDbAlignedDimension::jogSymbolOn() const { if (!impObj()) return false; return impObj()->jogSymbolOn(); }
bool  PyDbAlignedDimension::setJogSymbolOn(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setJogSymbolOn(v), L"AcDbAlignedDimension::setJogSymbolOn"); }
AcGePoint3d PyDbAlignedDimension::jogSymbolPosition() const { if (!impObj()) return AcGePoint3d(); return impObj()->jogSymbolPosition(); }
bool  PyDbAlignedDimension::setJogSymbolPosition(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setJogSymbolPosition(p), L"AcDbAlignedDimension::setJogSymbolPosition"); }

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

