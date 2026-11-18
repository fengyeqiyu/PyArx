#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbFace.h"

PyWrapper_Cpp(DbFace)
AcString PyDbFace::toString() const { return AcString(std::format("AcDbFace[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbFace::getVertexAt(Adesk::UInt16 i) const { if (!impObj()) return AcGePoint3d(); AcGePoint3d p; if(!PyWarnIfError(impObj()->getVertexAt(i, p), L"AcDbFace::getVertexAt")) return AcGePoint3d(); return p; }

bool  PyDbFace::setVertexAt(Adesk::UInt16 i, const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setVertexAt(i, p), L"AcDbFace::setVertexAt"); }

bool PyDbFace::isEdgeVisibleAt(Adesk::UInt16 i) const { if (!impObj()) return false; Adesk::Boolean v = Adesk::kFalse; if(!PyWarnIfError(impObj()->isEdgeVisibleAt(i, v), L"AcDbFace::isEdgeVisibleAt")) return false; return v != Adesk::kFalse; }
bool  PyDbFace::makeEdgeVisibleAt(Adesk::UInt16 i) { if (!impObj()) return false; return PyWarnIfError(impObj()->makeEdgeVisibleAt(i), L"AcDbFace::makeEdgeVisibleAt"); }
bool  PyDbFace::makeEdgeInvisibleAt(Adesk::UInt16 i) { if (!impObj()) return false; return PyWarnIfError(impObj()->makeEdgeInvisibleAt(i), L"AcDbFace::makeEdgeInvisibleAt"); }

void bind_PyDbFace(pym& m) {
	py::class_<PyDbFace, PyDbEntity>(m, "PyDbFace")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&, const AcGePoint3d&, const AcGePoint3d&, bool, bool, bool, bool>(),
			py::arg("pt0"), py::arg("pt1"), py::arg("pt2"),
			py::arg("e0vis") = true, py::arg("e1vis") = true, py::arg("e2vis") = true, py::arg("e3vis") = true)
		.def(py::init<const AcGePoint3d&, const AcGePoint3d&, const AcGePoint3d&, const AcGePoint3d&, bool, bool, bool, bool>(),
			py::arg("pt0"), py::arg("pt1"), py::arg("pt2"), py::arg("pt3"),
			py::arg("e0vis") = true, py::arg("e1vis") = true, py::arg("e2vis") = true, py::arg("e3vis") = true)
		.def(py::init<AcDbFace*>(), py::arg("object"))
		.def("getVertexAt", &PyDbFace::getVertexAt, py::arg("index"))
		.def("setVertexAt", &PyDbFace::setVertexAt, py::arg("index"), py::arg("p"))
		.def("isEdgeVisibleAt", &PyDbFace::isEdgeVisibleAt, py::arg("index"))
		.def("makeEdgeVisibleAt", &PyDbFace::makeEdgeVisibleAt, py::arg("index"))
		.def("makeEdgeInvisibleAt", &PyDbFace::makeEdgeInvisibleAt, py::arg("index"))
		PyWrapper_BIND(DbFace)
		.def("__repr__", &PyDbFace::toString)
		;
}

