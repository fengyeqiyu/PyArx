#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbCircle.h"

PyWrapper_Cpp(DbCircle)
AcString PyDbCircle::toString() const { return AcString(std::format("AcDbCircle[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbCircle::center() const { if (!impObj()) return AcGePoint3d(); return impObj()->center(); }

bool  PyDbCircle::setCenter(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setCenter(p), L"AcDbCircle::setCenter"); }

double PyDbCircle::radius() const { if (!impObj()) return 0.0; return impObj()->radius(); }

bool  PyDbCircle::setRadius(double r) { if (!impObj()) return false; return PyWarnIfError(impObj()->setRadius(r), L"AcDbCircle::setRadius"); }

AcGeVector3d PyDbCircle::normal() const { if (!impObj()) return AcGeVector3d(); return impObj()->normal(); }

bool  PyDbCircle::setNormal(const AcGeVector3d& n) { if (!impObj()) return false; return PyWarnIfError(impObj()->setNormal(n), L"AcDbCircle::setNormal"); }

double PyDbCircle::thickness() const { if (!impObj()) return 0.0; return impObj()->thickness(); }
bool  PyDbCircle::setThickness(double t) { if (!impObj()) return false; return PyWarnIfError(impObj()->setThickness(t), L"AcDbCircle::setThickness"); }
double PyDbCircle::circumference() const { if (!impObj()) return 0.0; return impObj()->circumference(); }
bool  PyDbCircle::setCircumference(double c) { if (!impObj()) return false; return PyWarnIfError(impObj()->setCircumference(c), L"AcDbCircle::setCircumference"); }
double PyDbCircle::diameter() const { if (!impObj()) return 0.0; return impObj()->diameter(); }
bool  PyDbCircle::setDiameter(double d) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDiameter(d), L"AcDbCircle::setDiameter"); }

void bind_PyDbCircle(pym& m) {
	py::class_<PyDbCircle, PyDbCurve>(m, "PyDbCircle")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&, const AcGeVector3d&, double>(), py::arg("cntr"), py::arg("nrm"), py::arg("radius"))
		.def(py::init<AcDbCircle*>(), py::arg("object"))
		.def("center", &PyDbCircle::center)
		.def("setCenter", &PyDbCircle::setCenter, py::arg("p"))
		.def("radius", &PyDbCircle::radius)
		.def("setRadius", &PyDbCircle::setRadius, py::arg("r"))
		.def("normal", &PyDbCircle::normal)
		.def("setNormal", &PyDbCircle::setNormal, py::arg("n"))
		.def("thickness", &PyDbCircle::thickness)
		.def("setThickness", &PyDbCircle::setThickness, py::arg("t"))
		.def("circumference", &PyDbCircle::circumference)
		.def("setCircumference", &PyDbCircle::setCircumference, py::arg("c"))
		.def("diameter", &PyDbCircle::diameter)
		.def("setDiameter", &PyDbCircle::setDiameter, py::arg("d"))
		PyWrapper_BIND(DbCircle)
		.def("__repr__", &PyDbCircle::toString)
		;
}

