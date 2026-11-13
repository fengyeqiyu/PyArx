#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbPolygonMeshVertex.h"

PyWrapper_Cpp(DbPolygonMeshVertex);
AcString PyDbPolygonMeshVertex::toString() const { return AcString(std::format("AcDbPolygonMeshVertex[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbPolygonMeshVertex::position() const { return impObj()->position(); }
bool PyDbPolygonMeshVertex::setPosition(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setPosition(p), L"AcDbPolygonMeshVertex::setPosition"); }
int PyDbPolygonMeshVertex::vertexType() const { return static_cast<int>(impObj()->vertexType()); }

void bind_PyDbPolygonMeshVertex(pym& m) {
	py::class_<PyDbPolygonMeshVertex, PyDbVertex>(m, "PyDbPolygonMeshVertex")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&>(), py::arg("position"))
		.def(py::init<AcDbPolygonMeshVertex*>(), py::arg("object"))
		.def("position", &PyDbPolygonMeshVertex::position)
		.def("setPosition", &PyDbPolygonMeshVertex::setPosition, py::arg("p"))
		.def("vertexType", &PyDbPolygonMeshVertex::vertexType)
		PyWrapper_BIND(DbPolygonMeshVertex)
		.def("__repr__", &PyDbPolygonMeshVertex::toString)
		;
}
