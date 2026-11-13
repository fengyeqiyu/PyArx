#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbTrace.h"

PyWrapper_Cpp(DbTrace)
AcString PyDbTrace::toString() const { return AcString(std::format("AcDbTrace[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbTrace::getPointAt(Adesk::UInt16 i) const { AcGePoint3d p; if(!PyWarnIfError(impObj()->getPointAt(i, p), L"AcDbTrace::getPointAt")) return AcGePoint3d(); return p; }

bool  PyDbTrace::setPointAt(Adesk::UInt16 i, const AcGePoint3d& p) { return PyWarnIfError(impObj()->setPointAt(i, p), L"AcDbTrace::setPointAt"); }

double PyDbTrace::thickness() const { return impObj()->thickness(); }

bool  PyDbTrace::setThickness(double t) { return PyWarnIfError(impObj()->setThickness(t), L"AcDbTrace::setThickness"); }

AcGeVector3d PyDbTrace::normal() const { return impObj()->normal(); }
bool  PyDbTrace::setNormal(const AcGeVector3d& n) { return PyWarnIfError(impObj()->setNormal(n), L"AcDbTrace::setNormal"); }
// 注：AcDbTrace::elevation/setElevation 未被导入库导出（LNK2001），不绑定

void bind_PyDbTrace(pym& m) {
	py::class_<PyDbTrace, PyDbEntity>(m, "PyDbTrace")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&, const AcGePoint3d&, const AcGePoint3d&, const AcGePoint3d&>(), py::arg("pt0"), py::arg("pt1"), py::arg("pt2"), py::arg("pt3"))
		.def(py::init<AcDbTrace*>(), py::arg("object"))
		.def("getPointAt", &PyDbTrace::getPointAt, py::arg("index"))
		.def("setPointAt", &PyDbTrace::setPointAt, py::arg("index"), py::arg("p"))
		.def("thickness", &PyDbTrace::thickness)
		.def("setThickness", &PyDbTrace::setThickness, py::arg("t"))
		.def("normal", &PyDbTrace::normal)
		.def("setNormal", &PyDbTrace::setNormal, py::arg("n"))
		PyWrapper_BIND(DbTrace)
		.def("__repr__", &PyDbTrace::toString)
		;
}

