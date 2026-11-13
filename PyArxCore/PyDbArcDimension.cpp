#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbArcDimension.h"

PyWrapper_Cpp(DbArcDimension)
AcString PyDbArcDimension::toString() const { return AcString(std::format("AcDbArcDimension[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbArcDimension::arcPoint() const { return impObj()->arcPoint(); }

bool  PyDbArcDimension::setArcPoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setArcPoint(p), L"AcDbArcDimension::setArcPoint"); }

AcGePoint3d PyDbArcDimension::centerPoint() const { return impObj()->centerPoint(); }

bool  PyDbArcDimension::setCenterPoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setCenterPoint(p), L"AcDbArcDimension::setCenterPoint"); }

AcGePoint3d PyDbArcDimension::xLine1Point() const { return impObj()->xLine1Point(); }

bool  PyDbArcDimension::setXLine1Point(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setXLine1Point(p), L"AcDbArcDimension::setXLine1Point"); }

AcGePoint3d PyDbArcDimension::xLine2Point() const { return impObj()->xLine2Point(); }

bool  PyDbArcDimension::setXLine2Point(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setXLine2Point(p), L"AcDbArcDimension::setXLine2Point"); }

bool PyDbArcDimension::isPartial() const { return impObj()->isPartial(); }

bool  PyDbArcDimension::setIsPartial(bool v) { return PyWarnIfError(impObj()->setIsPartial(v), L"AcDbArcDimension::setIsPartial"); }

double PyDbArcDimension::arcStartParam() const { return impObj()->arcStartParam(); }
bool  PyDbArcDimension::setArcStartParam(double v) { return PyWarnIfError(impObj()->setArcStartParam(v), L"AcDbArcDimension::setArcStartParam"); }
double PyDbArcDimension::arcEndParam() const { return impObj()->arcEndParam(); }
bool  PyDbArcDimension::setArcEndParam(double v) { return PyWarnIfError(impObj()->setArcEndParam(v), L"AcDbArcDimension::setArcEndParam"); }
bool PyDbArcDimension::hasLeader() const { return impObj()->hasLeader(); }
bool  PyDbArcDimension::setHasLeader(bool v) { return PyWarnIfError(impObj()->setHasLeader(v), L"AcDbArcDimension::setHasLeader"); }
AcGePoint3d PyDbArcDimension::leader1Point() const { return impObj()->leader1Point(); }
bool  PyDbArcDimension::setLeader1Point(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setLeader1Point(p), L"AcDbArcDimension::setLeader1Point"); }
AcGePoint3d PyDbArcDimension::leader2Point() const { return impObj()->leader2Point(); }
bool  PyDbArcDimension::setLeader2Point(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setLeader2Point(p), L"AcDbArcDimension::setLeader2Point"); }
int PyDbArcDimension::arcSymbolType() const { return impObj()->arcSymbolType(); }
bool  PyDbArcDimension::setArcSymbolType(int v) { return PyWarnIfError(impObj()->setArcSymbolType(v), L"AcDbArcDimension::setArcSymbolType"); }

void bind_PyDbArcDimension(pym& m) {
	py::class_<PyDbArcDimension, PyDbDimension>(m, "PyDbArcDimension")
		.def(py::init<>())
		.def(py::init<AcDbArcDimension*>(), py::arg("object"))
		.def("arcPoint", &PyDbArcDimension::arcPoint)
		.def("setArcPoint", &PyDbArcDimension::setArcPoint, py::arg("p"))
		.def("centerPoint", &PyDbArcDimension::centerPoint)
		.def("setCenterPoint", &PyDbArcDimension::setCenterPoint, py::arg("p"))
		.def("xLine1Point", &PyDbArcDimension::xLine1Point)
		.def("setXLine1Point", &PyDbArcDimension::setXLine1Point, py::arg("p"))
		.def("xLine2Point", &PyDbArcDimension::xLine2Point)
		.def("setXLine2Point", &PyDbArcDimension::setXLine2Point, py::arg("p"))
		.def("isPartial", &PyDbArcDimension::isPartial)
		.def("setIsPartial", &PyDbArcDimension::setIsPartial, py::arg("v"))
		.def("arcStartParam", &PyDbArcDimension::arcStartParam)
		.def("setArcStartParam", &PyDbArcDimension::setArcStartParam, py::arg("v"))
		.def("arcEndParam", &PyDbArcDimension::arcEndParam)
		.def("setArcEndParam", &PyDbArcDimension::setArcEndParam, py::arg("v"))
		.def("hasLeader", &PyDbArcDimension::hasLeader)
		.def("setHasLeader", &PyDbArcDimension::setHasLeader, py::arg("v"))
		.def("leader1Point", &PyDbArcDimension::leader1Point)
		.def("setLeader1Point", &PyDbArcDimension::setLeader1Point, py::arg("p"))
		.def("leader2Point", &PyDbArcDimension::leader2Point)
		.def("setLeader2Point", &PyDbArcDimension::setLeader2Point, py::arg("p"))
		.def("arcSymbolType", &PyDbArcDimension::arcSymbolType)
		.def("setArcSymbolType", &PyDbArcDimension::setArcSymbolType, py::arg("v"))
		PyWrapper_BIND(DbArcDimension)
		.def("__repr__", &PyDbArcDimension::toString)
		;
}

