#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbOrdinateDimension.h"

PyWrapper_Cpp(DbOrdinateDimension)
AcString PyDbOrdinateDimension::toString() const { return AcString(std::format("AcDbOrdinateDimension[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbOrdinateDimension::definingPoint() const { if (!impObj()) return AcGePoint3d(); return impObj()->definingPoint(); }

bool  PyDbOrdinateDimension::setDefiningPoint(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDefiningPoint(p), L"AcDbOrdinateDimension::setDefiningPoint"); }

AcGePoint3d PyDbOrdinateDimension::leaderEndPoint() const { if (!impObj()) return AcGePoint3d(); return impObj()->leaderEndPoint(); }

bool  PyDbOrdinateDimension::setLeaderEndPoint(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setLeaderEndPoint(p), L"AcDbOrdinateDimension::setLeaderEndPoint"); }

AcGePoint3d PyDbOrdinateDimension::origin() const { if (!impObj()) return AcGePoint3d(); return impObj()->origin(); }

bool  PyDbOrdinateDimension::setOrigin(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setOrigin(p), L"AcDbOrdinateDimension::setOrigin"); }

bool PyDbOrdinateDimension::isUsingXAxis() const { if (!impObj()) return false; return impObj()->isUsingXAxis() != Adesk::kFalse; }
bool PyDbOrdinateDimension::isUsingYAxis() const { if (!impObj()) return false; return impObj()->isUsingYAxis() != Adesk::kFalse; }
bool  PyDbOrdinateDimension::useXAxis() { if (!impObj()) return false; return PyWarnIfError(impObj()->useXAxis(), L"AcDbOrdinateDimension::useXAxis"); }
bool  PyDbOrdinateDimension::useYAxis() { if (!impObj()) return false; return PyWarnIfError(impObj()->useYAxis(), L"AcDbOrdinateDimension::useYAxis"); }
bool  PyDbOrdinateDimension::setUsingXAxis(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setUsingXAxis(v), L"AcDbOrdinateDimension::setUsingXAxis"); }
bool  PyDbOrdinateDimension::setUsingYAxis(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setUsingYAxis(v), L"AcDbOrdinateDimension::setUsingYAxis"); }

void bind_PyDbOrdinateDimension(pym& m) {
	py::class_<PyDbOrdinateDimension, PyDbDimension>(m, "PyDbOrdinateDimension")
		.def(py::init<>())
		.def(py::init<AcDbOrdinateDimension*>(), py::arg("object"))
		.def("definingPoint", &PyDbOrdinateDimension::definingPoint)
		.def("setDefiningPoint", &PyDbOrdinateDimension::setDefiningPoint, py::arg("p"))
		.def("leaderEndPoint", &PyDbOrdinateDimension::leaderEndPoint)
		.def("setLeaderEndPoint", &PyDbOrdinateDimension::setLeaderEndPoint, py::arg("p"))
		.def("origin", &PyDbOrdinateDimension::origin)
		.def("setOrigin", &PyDbOrdinateDimension::setOrigin, py::arg("p"))
		.def("isUsingXAxis", &PyDbOrdinateDimension::isUsingXAxis)
		.def("isUsingYAxis", &PyDbOrdinateDimension::isUsingYAxis)
		.def("useXAxis", &PyDbOrdinateDimension::useXAxis)
		.def("useYAxis", &PyDbOrdinateDimension::useYAxis)
		.def("setUsingXAxis", &PyDbOrdinateDimension::setUsingXAxis, py::arg("v"))
		.def("setUsingYAxis", &PyDbOrdinateDimension::setUsingYAxis, py::arg("v"))
		PyWrapper_BIND(DbOrdinateDimension)
		.def("__repr__", &PyDbOrdinateDimension::toString)
		;
}

