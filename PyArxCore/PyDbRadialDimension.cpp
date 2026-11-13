#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbRadialDimension.h"

PyWrapper_Cpp(DbRadialDimension)
AcString PyDbRadialDimension::toString() const { return AcString(std::format("AcDbRadialDimension[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbRadialDimension::center() const { return impObj()->center(); }

bool  PyDbRadialDimension::setCenter(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setCenter(p), L"AcDbRadialDimension::setCenter"); }

AcGePoint3d PyDbRadialDimension::chordPoint() const { return impObj()->chordPoint(); }

bool  PyDbRadialDimension::setChordPoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setChordPoint(p), L"AcDbRadialDimension::setChordPoint"); }

double PyDbRadialDimension::leaderLength() const { return impObj()->leaderLength(); }

bool  PyDbRadialDimension::setLeaderLength(double l) { return PyWarnIfError(impObj()->setLeaderLength(l), L"AcDbRadialDimension::setLeaderLength"); }

double PyDbRadialDimension::extArcStartAngle() const { return impObj()->extArcStartAngle(); }
bool  PyDbRadialDimension::setExtArcStartAngle(double a) { return PyWarnIfError(impObj()->setExtArcStartAngle(a), L"AcDbRadialDimension::setExtArcStartAngle"); }
double PyDbRadialDimension::extArcEndAngle() const { return impObj()->extArcEndAngle(); }
bool  PyDbRadialDimension::setExtArcEndAngle(double a) { return PyWarnIfError(impObj()->setExtArcEndAngle(a), L"AcDbRadialDimension::setExtArcEndAngle"); }

void bind_PyDbRadialDimension(pym& m) {
	py::class_<PyDbRadialDimension, PyDbDimension>(m, "PyDbRadialDimension")
		.def(py::init<>())
		.def(py::init<AcDbRadialDimension*>(), py::arg("object"))
		.def("center", &PyDbRadialDimension::center)
		.def("setCenter", &PyDbRadialDimension::setCenter, py::arg("p"))
		.def("chordPoint", &PyDbRadialDimension::chordPoint)
		.def("setChordPoint", &PyDbRadialDimension::setChordPoint, py::arg("p"))
		.def("leaderLength", &PyDbRadialDimension::leaderLength)
		.def("setLeaderLength", &PyDbRadialDimension::setLeaderLength, py::arg("l"))
		.def("extArcStartAngle", &PyDbRadialDimension::extArcStartAngle)
		.def("setExtArcStartAngle", &PyDbRadialDimension::setExtArcStartAngle, py::arg("a"))
		.def("extArcEndAngle", &PyDbRadialDimension::extArcEndAngle)
		.def("setExtArcEndAngle", &PyDbRadialDimension::setExtArcEndAngle, py::arg("a"))
		PyWrapper_BIND(DbRadialDimension)
		.def("__repr__", &PyDbRadialDimension::toString)
		;
}

