#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbRay.h"

PyWrapper_Cpp(DbRay)
AcString PyDbRay::toString() const { return AcString(std::format("AcDbRay[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbRay::basePoint() const { return impObj() ? impObj()->basePoint() : AcGePoint3d(); }

void PyDbRay::setBasePoint(const AcGePoint3d& p) { if (!impObj()) return; impObj()->setBasePoint(p); }

AcGeVector3d PyDbRay::unitDir() const { return impObj() ? impObj()->unitDir() : AcGeVector3d(); }

void PyDbRay::setUnitDir(const AcGeVector3d& v) { if (!impObj()) return; impObj()->setUnitDir(v); }

AcGePoint3d PyDbRay::secondPoint() const { return impObj() ? impObj()->secondPoint() : AcGePoint3d(); }
bool  PyDbRay::setSecondPoint(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setSecondPoint(p), L"AcDbRay::setSecondPoint"); }

void bind_PyDbRay(pym& m) {
	py::class_<PyDbRay, PyDbCurve>(m, "PyDbRay")
		.def(py::init<>())
		.def(py::init<AcDbRay*>(), py::arg("object"))
		.def("basePoint", &PyDbRay::basePoint)
		.def("setBasePoint", &PyDbRay::setBasePoint, py::arg("p"))
		.def("unitDir", &PyDbRay::unitDir)
		.def("setUnitDir", &PyDbRay::setUnitDir, py::arg("v"))
		.def("secondPoint", &PyDbRay::secondPoint)
		.def("setSecondPoint", &PyDbRay::setSecondPoint, py::arg("p"))
		PyWrapper_BIND(DbRay)
		.def("__repr__", &PyDbRay::toString)
		;
}

