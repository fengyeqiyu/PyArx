#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbPolyFaceMeshVertex.h"

PyWrapper_Cpp(DbPolyFaceMeshVertex);
AcString PyDbPolyFaceMeshVertex::toString() const { return AcString(std::format("AcDbPolyFaceMeshVertex[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbPolyFaceMeshVertex::position() const { if (!impObj()) return AcGePoint3d(); return impObj() ? impObj()->position() : AcGePoint3d(); }
bool PyDbPolyFaceMeshVertex::setPosition(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setPosition(p), L"AcDbPolyFaceMeshVertex::setPosition"); }

void bind_PyDbPolyFaceMeshVertex(pym& m) {
	py::class_<PyDbPolyFaceMeshVertex, PyDbVertex>(m, "PyDbPolyFaceMeshVertex")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&>(), py::arg("position"))
		.def(py::init<AcDbPolyFaceMeshVertex*>(), py::arg("object"))
		.def("position", &PyDbPolyFaceMeshVertex::position)
		.def("setPosition", &PyDbPolyFaceMeshVertex::setPosition, py::arg("p"))
		PyWrapper_BIND(DbPolyFaceMeshVertex)
		.def("__repr__", &PyDbPolyFaceMeshVertex::toString)
		;
}
