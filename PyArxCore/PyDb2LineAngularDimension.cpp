#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDb2LineAngularDimension.h"

PyWrapper_Cpp(Db2LineAngularDimension)
AcString PyDb2LineAngularDimension::toString() const { return AcString(std::format("AcDb2LineAngularDimension[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDb2LineAngularDimension::arcPoint() const { if (!impObj()) return AcGePoint3d(); return impObj()->arcPoint(); }

bool PyDb2LineAngularDimension::setArcPoint(const AcGePoint3d& p)  { if (!impObj()) return false; return PyWarnIfError(impObj()->setArcPoint(p), L"AcDb2LineAngularDimension::setArcPoint"); }

AcGePoint3d PyDb2LineAngularDimension::xLine1Start() const { if (!impObj()) return AcGePoint3d(); return impObj()->xLine1Start(); }

bool PyDb2LineAngularDimension::setXLine1Start(const AcGePoint3d& p)  { if (!impObj()) return false; return PyWarnIfError(impObj()->setXLine1Start(p), L"AcDb2LineAngularDimension::setXLine1Start"); }

AcGePoint3d PyDb2LineAngularDimension::xLine1End() const { if (!impObj()) return AcGePoint3d(); return impObj()->xLine1End(); }

bool PyDb2LineAngularDimension::setXLine1End(const AcGePoint3d& p)  { if (!impObj()) return false; return PyWarnIfError(impObj()->setXLine1End(p), L"AcDb2LineAngularDimension::setXLine1End"); }

AcGePoint3d PyDb2LineAngularDimension::xLine2Start() const { if (!impObj()) return AcGePoint3d(); return impObj()->xLine2Start(); }

bool PyDb2LineAngularDimension::setXLine2Start(const AcGePoint3d& p)  { if (!impObj()) return false; return PyWarnIfError(impObj()->setXLine2Start(p), L"AcDb2LineAngularDimension::setXLine2Start"); }

AcGePoint3d PyDb2LineAngularDimension::xLine2End() const { if (!impObj()) return AcGePoint3d(); return impObj()->xLine2End(); }

bool PyDb2LineAngularDimension::setXLine2End(const AcGePoint3d& p)  { if (!impObj()) return false; return PyWarnIfError(impObj()->setXLine2End(p), L"AcDb2LineAngularDimension::setXLine2End"); }

bool PyDb2LineAngularDimension::extArcOn() const { if (!impObj()) return false; return impObj()->extArcOn(); }
bool PyDb2LineAngularDimension::setExtArcOn(bool v)  { if (!impObj()) return false; return PyWarnIfError(impObj()->setExtArcOn(v), L"AcDb2LineAngularDimension::setExtArcOn"); }

void bind_PyDb2LineAngularDimension(pym& m) {
	py::class_<PyDb2LineAngularDimension, PyDbDimension>(m, "PyDb2LineAngularDimension")
		.def(py::init<>())
		.def(py::init<AcDb2LineAngularDimension*>(), py::arg("object"))
		.def("arcPoint", &PyDb2LineAngularDimension::arcPoint)
		.def("setArcPoint", &PyDb2LineAngularDimension::setArcPoint, py::arg("p"))
		.def("xLine1Start", &PyDb2LineAngularDimension::xLine1Start)
		.def("setXLine1Start", &PyDb2LineAngularDimension::setXLine1Start, py::arg("p"))
		.def("xLine1End", &PyDb2LineAngularDimension::xLine1End)
		.def("setXLine1End", &PyDb2LineAngularDimension::setXLine1End, py::arg("p"))
		.def("xLine2Start", &PyDb2LineAngularDimension::xLine2Start)
		.def("setXLine2Start", &PyDb2LineAngularDimension::setXLine2Start, py::arg("p"))
		.def("xLine2End", &PyDb2LineAngularDimension::xLine2End)
		.def("setXLine2End", &PyDb2LineAngularDimension::setXLine2End, py::arg("p"))
		.def("extArcOn", &PyDb2LineAngularDimension::extArcOn)
		.def("setExtArcOn", &PyDb2LineAngularDimension::setExtArcOn, py::arg("v"))
		PyWrapper_BIND(Db2LineAngularDimension)
		.def("__repr__", &PyDb2LineAngularDimension::toString)
		;
}

