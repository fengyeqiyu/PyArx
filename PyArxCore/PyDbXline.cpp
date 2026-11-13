#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbXline.h"

PyWrapper_Cpp(DbXline)
AcString PyDbXline::toString() const { return AcString(std::format("AcDbXline[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbXline::basePoint() const { return impObj()->basePoint(); }

void PyDbXline::setBasePoint(const AcGePoint3d& p) { impObj()->setBasePoint(p); }

AcGeVector3d PyDbXline::unitDir() const { return impObj()->unitDir(); }

void PyDbXline::setUnitDir(const AcGeVector3d& v) { impObj()->setUnitDir(v); }

AcGePoint3d PyDbXline::secondPoint() const { return impObj()->secondPoint(); }
bool  PyDbXline::setSecondPoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setSecondPoint(p), L"AcDbXline::setSecondPoint"); }

void bind_PyDbXline(pym& m) {
	py::class_<PyDbXline, PyDbCurve>(m, "PyDbXline")
		.def(py::init<>())
		.def(py::init<AcDbXline*>(), py::arg("object"))
		.def("basePoint", &PyDbXline::basePoint)
		.def("setBasePoint", &PyDbXline::setBasePoint, py::arg("p"))
		.def("unitDir", &PyDbXline::unitDir)
		.def("setUnitDir", &PyDbXline::setUnitDir, py::arg("v"))
		.def("secondPoint", &PyDbXline::secondPoint)
		.def("setSecondPoint", &PyDbXline::setSecondPoint, py::arg("p"))
		PyWrapper_BIND(DbXline)
		.def("__repr__", &PyDbXline::toString)
		;
}

