#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDb2dVertex.h"

PyWrapper_Cpp(Db2dVertex);
AcString PyDb2dVertex::toString() const { return AcString(std::format("AcDb2dVertex[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDb2dVertex::position() const { return impObj()->position(); }
bool PyDb2dVertex::setPosition(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setPosition(p), L"AcDb2dVertex::setPosition"); }
double PyDb2dVertex::startWidth() const { return impObj()->startWidth(); }
bool PyDb2dVertex::setStartWidth(double v) { return PyWarnIfError(impObj()->setStartWidth(v), L"AcDb2dVertex::setStartWidth"); }
double PyDb2dVertex::endWidth() const { return impObj()->endWidth(); }
bool PyDb2dVertex::setEndWidth(double v) { return PyWarnIfError(impObj()->setEndWidth(v), L"AcDb2dVertex::setEndWidth"); }
double PyDb2dVertex::bulge() const { return impObj()->bulge(); }
bool PyDb2dVertex::setBulge(double v) { return PyWarnIfError(impObj()->setBulge(v), L"AcDb2dVertex::setBulge"); }
bool PyDb2dVertex::isTangentUsed() const { return impObj()->isTangentUsed() != Adesk::kFalse; }
bool PyDb2dVertex::useTangent() { return PyWarnIfError(impObj()->useTangent(), L"AcDb2dVertex::useTangent"); }
bool PyDb2dVertex::ignoreTangent() { return PyWarnIfError(impObj()->ignoreTangent(), L"AcDb2dVertex::ignoreTangent"); }
bool PyDb2dVertex::setTangentUsed(bool v) { return PyWarnIfError(impObj()->setTangentUsed(v ? Adesk::kTrue : Adesk::kFalse), L"AcDb2dVertex::setTangentUsed"); }
double PyDb2dVertex::tangent() const { return impObj()->tangent(); }
bool PyDb2dVertex::setTangent(double v) { return PyWarnIfError(impObj()->setTangent(v), L"AcDb2dVertex::setTangent"); }
int PyDb2dVertex::vertexType() const { return static_cast<int>(impObj()->vertexType()); }

void bind_PyDb2dVertex(pym& m) {
	py::class_<PyDb2dVertex, PyDbVertex>(m, "PyDb2dVertex")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&>(), py::arg("position"))
		.def(py::init<AcDb2dVertex*>(), py::arg("object"))
		.def("position", &PyDb2dVertex::position)
		.def("setPosition", &PyDb2dVertex::setPosition, py::arg("p"))
		.def("startWidth", &PyDb2dVertex::startWidth)
		.def("setStartWidth", &PyDb2dVertex::setStartWidth, py::arg("v"))
		.def("endWidth", &PyDb2dVertex::endWidth)
		.def("setEndWidth", &PyDb2dVertex::setEndWidth, py::arg("v"))
		.def("bulge", &PyDb2dVertex::bulge)
		.def("setBulge", &PyDb2dVertex::setBulge, py::arg("v"))
		.def("isTangentUsed", &PyDb2dVertex::isTangentUsed)
		.def("useTangent", &PyDb2dVertex::useTangent)
		.def("ignoreTangent", &PyDb2dVertex::ignoreTangent)
		.def("setTangentUsed", &PyDb2dVertex::setTangentUsed, py::arg("v"))
		.def("tangent", &PyDb2dVertex::tangent)
		.def("setTangent", &PyDb2dVertex::setTangent, py::arg("v"))
		.def("vertexType", &PyDb2dVertex::vertexType)
		PyWrapper_BIND(Db2dVertex)
		.def("__repr__", &PyDb2dVertex::toString)
		;
}
