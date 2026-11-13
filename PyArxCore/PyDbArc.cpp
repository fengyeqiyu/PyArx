#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbArc.h"

PyWrapper_Cpp(DbArc)
AcString PyDbArc::toString() const { return AcString(std::format("AcDbArc[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbArc::center() const { return impObj()->center(); }

bool  PyDbArc::setCenter(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setCenter(p), L"AcDbArc::setCenter"); }

double PyDbArc::radius() const { return impObj()->radius(); }

bool  PyDbArc::setRadius(double r) { return PyWarnIfError(impObj()->setRadius(r), L"AcDbArc::setRadius"); }

double PyDbArc::startAngle() const { return impObj()->startAngle(); }

bool  PyDbArc::setStartAngle(double a) { return PyWarnIfError(impObj()->setStartAngle(a), L"AcDbArc::setStartAngle"); }

double PyDbArc::endAngle() const { return impObj()->endAngle(); }

bool  PyDbArc::setEndAngle(double a) { return PyWarnIfError(impObj()->setEndAngle(a), L"AcDbArc::setEndAngle"); }

double PyDbArc::totalAngle() const { return impObj()->totalAngle(); }
double PyDbArc::length() const { return impObj()->length(); }
double PyDbArc::thickness() const { return impObj()->thickness(); }
bool  PyDbArc::setThickness(double t) { return PyWarnIfError(impObj()->setThickness(t), L"AcDbArc::setThickness"); }
AcGeVector3d PyDbArc::normal() const { return impObj()->normal(); }
bool  PyDbArc::setNormal(const AcGeVector3d& n) { return PyWarnIfError(impObj()->setNormal(n), L"AcDbArc::setNormal"); }

void bind_PyDbArc(pym& m) {
	py::class_<PyDbArc, PyDbCurve>(m, "PyDbArc")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&, double, double, double>(), py::arg("center"), py::arg("radius"), py::arg("startAngle"), py::arg("endAngle"))
		.def(py::init<const AcGePoint3d&, const AcGeVector3d&, double, double, double>(), py::arg("center"), py::arg("normal"), py::arg("radius"), py::arg("startAngle"), py::arg("endAngle"))
		.def(py::init<AcDbArc*>(), py::arg("object"))
		.def("center", &PyDbArc::center)
		.def("setCenter", &PyDbArc::setCenter, py::arg("p"))
		.def("radius", &PyDbArc::radius)
		.def("setRadius", &PyDbArc::setRadius, py::arg("r"))
		.def("startAngle", &PyDbArc::startAngle)
		.def("setStartAngle", &PyDbArc::setStartAngle, py::arg("a"))
		.def("endAngle", &PyDbArc::endAngle)
		.def("setEndAngle", &PyDbArc::setEndAngle, py::arg("a"))
		.def("totalAngle", &PyDbArc::totalAngle)
		.def("length", &PyDbArc::length)
		.def("thickness", &PyDbArc::thickness)
		.def("setThickness", &PyDbArc::setThickness, py::arg("t"))
		.def("normal", &PyDbArc::normal)
		.def("setNormal", &PyDbArc::setNormal, py::arg("n"))
		PyWrapper_BIND(DbArc)
		.def("__repr__", &PyDbArc::toString)
		;
}

