#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDiametricDimension.h"

PyWrapper_Cpp(DbDiametricDimension)
AcString PyDbDiametricDimension::toString() const { return AcString(std::format("AcDbDiametricDimension[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbDiametricDimension::chordPoint() const { return impObj()->chordPoint(); }

bool  PyDbDiametricDimension::setChordPoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setChordPoint(p), L"AcDbDiametricDimension::setChordPoint"); }

AcGePoint3d PyDbDiametricDimension::farChordPoint() const { return impObj()->farChordPoint(); }

bool  PyDbDiametricDimension::setFarChordPoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setFarChordPoint(p), L"AcDbDiametricDimension::setFarChordPoint"); }

double PyDbDiametricDimension::leaderLength() const { return impObj()->leaderLength(); }

bool  PyDbDiametricDimension::setLeaderLength(double l) { return PyWarnIfError(impObj()->setLeaderLength(l), L"AcDbDiametricDimension::setLeaderLength"); }

double PyDbDiametricDimension::extArcStartAngle() const { return impObj()->extArcStartAngle(); }
bool  PyDbDiametricDimension::setExtArcStartAngle(double a) { return PyWarnIfError(impObj()->setExtArcStartAngle(a), L"AcDbDiametricDimension::setExtArcStartAngle"); }
double PyDbDiametricDimension::extArcEndAngle() const { return impObj()->extArcEndAngle(); }
bool  PyDbDiametricDimension::setExtArcEndAngle(double a) { return PyWarnIfError(impObj()->setExtArcEndAngle(a), L"AcDbDiametricDimension::setExtArcEndAngle"); }

void bind_PyDbDiametricDimension(pym& m) {
	py::class_<PyDbDiametricDimension, PyDbDimension>(m, "PyDbDiametricDimension")
		.def(py::init<>())
		.def(py::init<AcDbDiametricDimension*>(), py::arg("object"))
		.def("chordPoint", &PyDbDiametricDimension::chordPoint)
		.def("setChordPoint", &PyDbDiametricDimension::setChordPoint, py::arg("p"))
		.def("farChordPoint", &PyDbDiametricDimension::farChordPoint)
		.def("setFarChordPoint", &PyDbDiametricDimension::setFarChordPoint, py::arg("p"))
		.def("leaderLength", &PyDbDiametricDimension::leaderLength)
		.def("setLeaderLength", &PyDbDiametricDimension::setLeaderLength, py::arg("l"))
		.def("extArcStartAngle", &PyDbDiametricDimension::extArcStartAngle)
		.def("setExtArcStartAngle", &PyDbDiametricDimension::setExtArcStartAngle, py::arg("a"))
		.def("extArcEndAngle", &PyDbDiametricDimension::extArcEndAngle)
		.def("setExtArcEndAngle", &PyDbDiametricDimension::setExtArcEndAngle, py::arg("a"))
		PyWrapper_BIND(DbDiametricDimension)
		.def("__repr__", &PyDbDiametricDimension::toString)
		;
}

