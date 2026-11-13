#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDb3PointAngularDimension.h"

PyWrapper_Cpp(Db3PointAngularDimension)
AcString PyDb3PointAngularDimension::toString() const { return AcString(std::format("AcDb3PointAngularDimension[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDb3PointAngularDimension::centerPoint() const { return impObj()->centerPoint(); }

bool  PyDb3PointAngularDimension::setCenterPoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setCenterPoint(p), L"AcDb3PointAngularDimension::setCenterPoint"); }

AcGePoint3d PyDb3PointAngularDimension::arcPoint() const { return impObj()->arcPoint(); }

bool  PyDb3PointAngularDimension::setArcPoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setArcPoint(p), L"AcDb3PointAngularDimension::setArcPoint"); }

AcGePoint3d PyDb3PointAngularDimension::xLine1Point() const { return impObj()->xLine1Point(); }
bool  PyDb3PointAngularDimension::setXLine1Point(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setXLine1Point(p), L"AcDb3PointAngularDimension::setXLine1Point"); }
AcGePoint3d PyDb3PointAngularDimension::xLine2Point() const { return impObj()->xLine2Point(); }
bool  PyDb3PointAngularDimension::setXLine2Point(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setXLine2Point(p), L"AcDb3PointAngularDimension::setXLine2Point"); }
bool PyDb3PointAngularDimension::extArcOn() const { return impObj()->extArcOn(); }
bool  PyDb3PointAngularDimension::setExtArcOn(bool v) { return PyWarnIfError(impObj()->setExtArcOn(v), L"AcDb3PointAngularDimension::setExtArcOn"); }

void bind_PyDb3PointAngularDimension(pym& m) {
	py::class_<PyDb3PointAngularDimension, PyDbDimension>(m, "PyDbAngularDimension")
		.def(py::init<>())
		.def(py::init<AcDb3PointAngularDimension*>(), py::arg("object"))
		.def("centerPoint", &PyDb3PointAngularDimension::centerPoint)
		.def("setCenterPoint", &PyDb3PointAngularDimension::setCenterPoint, py::arg("p"))
		.def("arcPoint", &PyDb3PointAngularDimension::arcPoint)
		.def("setArcPoint", &PyDb3PointAngularDimension::setArcPoint, py::arg("p"))
		.def("xLine1Point", &PyDb3PointAngularDimension::xLine1Point)
		.def("setXLine1Point", &PyDb3PointAngularDimension::setXLine1Point, py::arg("p"))
		.def("xLine2Point", &PyDb3PointAngularDimension::xLine2Point)
		.def("setXLine2Point", &PyDb3PointAngularDimension::setXLine2Point, py::arg("p"))
		.def("extArcOn", &PyDb3PointAngularDimension::extArcOn)
		.def("setExtArcOn", &PyDb3PointAngularDimension::setExtArcOn, py::arg("v"))
		PyWrapper_BIND(Db3PointAngularDimension)
		.def("__repr__", &PyDb3PointAngularDimension::toString)
		;
}

