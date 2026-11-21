#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDb3dPolylineVertex.h"

PyWrapper_Cpp(Db3dPolylineVertex);
AcString PyDb3dPolylineVertex::toString() const { return AcString(std::format("AcDb3dPolylineVertex[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDb3dPolylineVertex::position() const { if (!impObj()) return AcGePoint3d(); return impObj() ? impObj()->position() : AcGePoint3d(); }
bool PyDb3dPolylineVertex::setPosition(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setPosition(p), L"AcDb3dPolylineVertex::setPosition"); }
int PyDb3dPolylineVertex::vertexType() const { if (!impObj()) return 0; return impObj() ? static_cast<int>(impObj()->vertexType()) : 0; }

void bind_PyDb3dPolylineVertex(pym& m) {
	py::class_<PyDb3dPolylineVertex, PyDbVertex>(m, "PyDb3dPolylineVertex")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&>(), py::arg("position"))
		.def(py::init<AcDb3dPolylineVertex*>(), py::arg("object"))
		.def("position", &PyDb3dPolylineVertex::position)
		.def("setPosition", &PyDb3dPolylineVertex::setPosition, py::arg("p"))
		.def("vertexType", &PyDb3dPolylineVertex::vertexType)
		PyWrapper_BIND(Db3dPolylineVertex)
		.def("__repr__", &PyDb3dPolylineVertex::toString)
		;
}
