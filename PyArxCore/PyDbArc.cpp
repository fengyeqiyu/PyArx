#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbArc.h"

PyWrapper_Cpp(DbArc)
AcString PyDbArc::toString() const { return AcString(std::format("AcDbArc[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbArc::center() const { return impObj() ? impObj()->center() : AcGePoint3d(); }

bool  PyDbArc::setCenter(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setCenter(p), L"AcDbArc::setCenter"); }

double PyDbArc::radius() const { return impObj() ? impObj()->radius() : 0.0; }

bool  PyDbArc::setRadius(double r) { if (!impObj()) return false; return PyWarnIfError(impObj()->setRadius(r), L"AcDbArc::setRadius"); }

double PyDbArc::startAngle() const { return impObj() ? impObj()->startAngle() : 0.0; }

bool  PyDbArc::setStartAngle(double a) { if (!impObj()) return false; return PyWarnIfError(impObj()->setStartAngle(a), L"AcDbArc::setStartAngle"); }

double PyDbArc::endAngle() const { return impObj() ? impObj()->endAngle() : 0.0; }

bool  PyDbArc::setEndAngle(double a) { if (!impObj()) return false; return PyWarnIfError(impObj()->setEndAngle(a), L"AcDbArc::setEndAngle"); }

double PyDbArc::totalAngle() const { return impObj() ? impObj()->totalAngle() : 0.0; }
double PyDbArc::length() const { return impObj() ? impObj()->length() : 0.0; }
double PyDbArc::thickness() const { return impObj() ? impObj()->thickness() : 0.0; }
bool  PyDbArc::setThickness(double t) { if (!impObj()) return false; return PyWarnIfError(impObj()->setThickness(t), L"AcDbArc::setThickness"); }
AcGeVector3d PyDbArc::normal() const { return impObj() ? impObj()->normal() : AcGeVector3d(); }
bool  PyDbArc::setNormal(const AcGeVector3d& n) { if (!impObj()) return false; return PyWarnIfError(impObj()->setNormal(n), L"AcDbArc::setNormal"); }

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

