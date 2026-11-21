#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSection.h"

PyWrapper_Cpp(DbSection)
AcString PyDbSection::toString() const { return AcString(std::format("AcDbSection[wrapper]").c_str(), AcString::Utf8); }

int PyDbSection::state() const { if (!impObj()) return 0; return impObj() ? static_cast<int>(impObj()->state()) : 0; }
bool  PyDbSection::setState(int v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setState(static_cast<AcDbSection::State>(v)), L"AcDbSection::setState"); }
AcString PyDbSection::getName() const { AcString s; if (!impObj()) return s; if(!PyWarnIfError(impObj()->getName(s), L"AcDbSection::getName")) return AcString(); return s; }
bool  PyDbSection::setName(const AcString& n) { if (!impObj()) return false; return PyWarnIfError(impObj()->setName(n.kwszPtr()), L"AcDbSection::setName"); }
AcGeVector3d PyDbSection::viewingDirection() const { if (!impObj()) return AcGeVector3d(); return impObj() ? impObj()->viewingDirection() : AcGeVector3d(); }
bool  PyDbSection::setViewingDirection(const AcGeVector3d& v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setViewingDirection(v), L"AcDbSection::setViewingDirection"); }
AcGeVector3d PyDbSection::verticalDirection() const { if (!impObj()) return AcGeVector3d(); return impObj() ? impObj()->verticalDirection() : AcGeVector3d(); }
bool  PyDbSection::setVerticalDirection(const AcGeVector3d& v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setVerticalDirection(v), L"AcDbSection::setVerticalDirection"); }
AcGeVector3d PyDbSection::normal() const { if (!impObj()) return AcGeVector3d(); return impObj() ? impObj()->normal() : AcGeVector3d(); }
int PyDbSection::indicatorTransparency() const { if (!impObj()) return 0; return impObj() ? impObj()->indicatorTransparency() : 0; }
bool  PyDbSection::setIndicatorTransparency(int v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setIndicatorTransparency(v), L"AcDbSection::setIndicatorTransparency"); }
int PyDbSection::numVertices() const { if (!impObj()) return 0; return impObj() ? impObj()->numVertices() : 0; }
AcGePoint3d PyDbSection::getVertex(int i) const { AcGePoint3d pt; if (!impObj()) return pt; if(!PyWarnIfError(impObj()->getVertex(i, pt), L"AcDbSection::getVertex")) return AcGePoint3d(); return pt; }
bool  PyDbSection::setVertex(int i, const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setVertex(i, p), L"AcDbSection::setVertex"); }
bool  PyDbSection::addVertex(int i, const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->addVertex(i, p), L"AcDbSection::addVertex"); }
bool  PyDbSection::removeVertex(int i) { if (!impObj()) return false; return PyWarnIfError(impObj()->removeVertex(i), L"AcDbSection::removeVertex"); }

void bind_PyDbSection(pym& m) {
	py::class_<PyDbSection, PyDbEntity>(m, "PyDbSection")
		.def(py::init<>())
		.def(py::init<AcDbSection*>(), py::arg("object"))
		.def("state", &PyDbSection::state)
		.def("setState", &PyDbSection::setState, py::arg("v"))
		.def("getName", &PyDbSection::getName)
		.def("setName", &PyDbSection::setName, py::arg("n"))
		.def("viewingDirection", &PyDbSection::viewingDirection)
		.def("setViewingDirection", &PyDbSection::setViewingDirection, py::arg("v"))
		.def("verticalDirection", &PyDbSection::verticalDirection)
		.def("setVerticalDirection", &PyDbSection::setVerticalDirection, py::arg("v"))
		.def("normal", &PyDbSection::normal)
		.def("indicatorTransparency", &PyDbSection::indicatorTransparency)
		.def("setIndicatorTransparency", &PyDbSection::setIndicatorTransparency, py::arg("v"))
		.def("numVertices", &PyDbSection::numVertices)
		.def("getVertex", &PyDbSection::getVertex, py::arg("index"))
		.def("setVertex", &PyDbSection::setVertex, py::arg("index"), py::arg("p"))
		.def("addVertex", &PyDbSection::addVertex, py::arg("index"), py::arg("p"))
		.def("removeVertex", &PyDbSection::removeVertex, py::arg("index"))
		PyWrapper_BIND(DbSection)
		.def("__repr__", &PyDbSection::toString);
}
