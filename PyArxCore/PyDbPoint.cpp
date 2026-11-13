#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbPoint.h"

PyWrapper_Cpp(DbPoint)
AcString PyDbPoint::toString() const { return AcString(std::format("AcDbPoint[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbPoint::position() const { return impObj()->position(); }

bool  PyDbPoint::setPosition(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setPosition(p), L"AcDbPoint::setPosition"); }

double PyDbPoint::thickness() const { return impObj()->thickness(); }

bool  PyDbPoint::setThickness(double t) { return PyWarnIfError(impObj()->setThickness(t), L"AcDbPoint::setThickness"); }

AcGeVector3d PyDbPoint::normal() const { return impObj()->normal(); }
bool  PyDbPoint::setNormal(const AcGeVector3d& n) { return PyWarnIfError(impObj()->setNormal(n), L"AcDbPoint::setNormal"); }
double PyDbPoint::ecsRotation() const { return impObj()->ecsRotation(); }
bool  PyDbPoint::setEcsRotation(double r) { return PyWarnIfError(impObj()->setEcsRotation(r), L"AcDbPoint::setEcsRotation"); }

void bind_PyDbPoint(pym& m) {
	py::class_<PyDbPoint, PyDbEntity>(m, "PyDbPoint")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&>(), py::arg("position"))
		.def(py::init<AcDbPoint*>(), py::arg("object"))
		.def("position", &PyDbPoint::position)
		.def("setPosition", &PyDbPoint::setPosition, py::arg("p"))
		.def("thickness", &PyDbPoint::thickness)
		.def("setThickness", &PyDbPoint::setThickness, py::arg("t"))
		.def("normal", &PyDbPoint::normal)
		.def("setNormal", &PyDbPoint::setNormal, py::arg("n"))
		.def("ecsRotation", &PyDbPoint::ecsRotation)
		.def("setEcsRotation", &PyDbPoint::setEcsRotation, py::arg("r"))
		PyWrapper_BIND(DbPoint)
		.def("__repr__", &PyDbPoint::toString)
		;
}

