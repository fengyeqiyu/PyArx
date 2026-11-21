#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSolid.h"

PyWrapper_Cpp(DbSolid)
AcString PyDbSolid::toString() const { return AcString(std::format("AcDbSolid[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbSolid::getPointAt(Adesk::UInt16 i) const { if (!impObj()) return AcGePoint3d(); AcGePoint3d p; if(!PyWarnIfError(impObj()->getPointAt(i, p), L"AcDbSolid::getPointAt")) return AcGePoint3d(); return p; }

bool  PyDbSolid::setPointAt(Adesk::UInt16 i, const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setPointAt(i, p), L"AcDbSolid::setPointAt"); }

double PyDbSolid::thickness() const { if (!impObj()) return 0.0; return impObj() ? impObj()->thickness() : 0.0; }

bool  PyDbSolid::setThickness(double t) { if (!impObj()) return false; return PyWarnIfError(impObj()->setThickness(t), L"AcDbSolid::setThickness"); }

AcGeVector3d PyDbSolid::normal() const { if (!impObj()) return AcGeVector3d(); return impObj() ? impObj()->normal() : AcGeVector3d(); }
bool  PyDbSolid::setNormal(const AcGeVector3d& n) { if (!impObj()) return false; return PyWarnIfError(impObj()->setNormal(n), L"AcDbSolid::setNormal"); }
// 注：AcDbSolid::elevation/setElevation 未被导入库导出（LNK2001），不绑定

void bind_PyDbSolid(pym& m) {
	py::class_<PyDbSolid, PyDbEntity>(m, "PyDbSolid")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&, const AcGePoint3d&, const AcGePoint3d&>(), py::arg("pt0"), py::arg("pt1"), py::arg("pt2"))
		.def(py::init<const AcGePoint3d&, const AcGePoint3d&, const AcGePoint3d&, const AcGePoint3d&>(), py::arg("pt0"), py::arg("pt1"), py::arg("pt2"), py::arg("pt3"))
		.def(py::init<AcDbSolid*>(), py::arg("object"))
		.def("getPointAt", &PyDbSolid::getPointAt, py::arg("index"))
		.def("setPointAt", &PyDbSolid::setPointAt, py::arg("index"), py::arg("p"))
		.def("thickness", &PyDbSolid::thickness)
		.def("setThickness", &PyDbSolid::setThickness, py::arg("t"))
		.def("normal", &PyDbSolid::normal)
		.def("setNormal", &PyDbSolid::setNormal, py::arg("n"))
		PyWrapper_BIND(DbSolid)
		.def("__repr__", &PyDbSolid::toString)
		;
}

