#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbRadialDimensionLarge.h"

PyWrapper_Cpp(DbRadialDimensionLarge)
AcString PyDbRadialDimensionLarge::toString() const { return AcString(std::format("AcDbRadialDimensionLarge[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbRadialDimensionLarge::center() const { return impObj()->center(); }

bool  PyDbRadialDimensionLarge::setCenter(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setCenter(p), L"AcDbRadialDimensionLarge::setCenter"); }

AcGePoint3d PyDbRadialDimensionLarge::chordPoint() const { return impObj()->chordPoint(); }

bool  PyDbRadialDimensionLarge::setChordPoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setChordPoint(p), L"AcDbRadialDimensionLarge::setChordPoint"); }

AcGePoint3d PyDbRadialDimensionLarge::overrideCenter() const { return impObj()->overrideCenter(); }

bool  PyDbRadialDimensionLarge::setOverrideCenter(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setOverrideCenter(p), L"AcDbRadialDimensionLarge::setOverrideCenter"); }

AcGePoint3d PyDbRadialDimensionLarge::jogPoint() const { return impObj()->jogPoint(); }

bool  PyDbRadialDimensionLarge::setJogPoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setJogPoint(p), L"AcDbRadialDimensionLarge::setJogPoint"); }

double PyDbRadialDimensionLarge::jogAngle() const { return impObj()->jogAngle(); }

bool  PyDbRadialDimensionLarge::setJogAngle(double a) { return PyWarnIfError(impObj()->setJogAngle(a), L"AcDbRadialDimensionLarge::setJogAngle"); }

bool  PyDbRadialDimensionLarge::setOverrideCenterPP(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setOverrideCenterPP(p), L"AcDbRadialDimensionLarge::setOverrideCenterPP"); }
bool  PyDbRadialDimensionLarge::setJogPointPP(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setJogPointPP(p), L"AcDbRadialDimensionLarge::setJogPointPP"); }
bool  PyDbRadialDimensionLarge::setTextPositionPP(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setTextPositionPP(p), L"AcDbRadialDimensionLarge::setTextPositionPP"); }
double PyDbRadialDimensionLarge::extArcStartAngle() const { return impObj()->extArcStartAngle(); }
bool  PyDbRadialDimensionLarge::setExtArcStartAngle(double a) { return PyWarnIfError(impObj()->setExtArcStartAngle(a), L"AcDbRadialDimensionLarge::setExtArcStartAngle"); }
double PyDbRadialDimensionLarge::extArcEndAngle() const { return impObj()->extArcEndAngle(); }
bool  PyDbRadialDimensionLarge::setExtArcEndAngle(double a) { return PyWarnIfError(impObj()->setExtArcEndAngle(a), L"AcDbRadialDimensionLarge::setExtArcEndAngle"); }

void bind_PyDbRadialDimensionLarge(pym& m) {
	py::class_<PyDbRadialDimensionLarge, PyDbDimension>(m, "PyDbRadialDimensionLarge")
		.def(py::init<>())
		.def(py::init<AcDbRadialDimensionLarge*>(), py::arg("object"))
		.def("center", &PyDbRadialDimensionLarge::center)
		.def("setCenter", &PyDbRadialDimensionLarge::setCenter, py::arg("p"))
		.def("chordPoint", &PyDbRadialDimensionLarge::chordPoint)
		.def("setChordPoint", &PyDbRadialDimensionLarge::setChordPoint, py::arg("p"))
		.def("overrideCenter", &PyDbRadialDimensionLarge::overrideCenter)
		.def("setOverrideCenter", &PyDbRadialDimensionLarge::setOverrideCenter, py::arg("p"))
		.def("jogPoint", &PyDbRadialDimensionLarge::jogPoint)
		.def("setJogPoint", &PyDbRadialDimensionLarge::setJogPoint, py::arg("p"))
		.def("jogAngle", &PyDbRadialDimensionLarge::jogAngle)
		.def("setJogAngle", &PyDbRadialDimensionLarge::setJogAngle, py::arg("a"))
		.def("setOverrideCenterPP", &PyDbRadialDimensionLarge::setOverrideCenterPP, py::arg("p"))
		.def("setJogPointPP", &PyDbRadialDimensionLarge::setJogPointPP, py::arg("p"))
		.def("setTextPositionPP", &PyDbRadialDimensionLarge::setTextPositionPP, py::arg("p"))
		.def("extArcStartAngle", &PyDbRadialDimensionLarge::extArcStartAngle)
		.def("setExtArcStartAngle", &PyDbRadialDimensionLarge::setExtArcStartAngle, py::arg("a"))
		.def("extArcEndAngle", &PyDbRadialDimensionLarge::extArcEndAngle)
		.def("setExtArcEndAngle", &PyDbRadialDimensionLarge::setExtArcEndAngle, py::arg("a"))
		PyWrapper_BIND(DbRadialDimensionLarge)
		.def("__repr__", &PyDbRadialDimensionLarge::toString)
		;
}

