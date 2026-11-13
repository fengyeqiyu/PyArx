#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbShape.h"

PyWrapper_Cpp(DbShape)
AcString PyDbShape::toString() const { return AcString(std::format("AcDbShape[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbShape::position() const { return impObj()->position(); }

bool  PyDbShape::setPosition(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setPosition(p), L"AcDbShape::setPosition"); }

double PyDbShape::size() const { return impObj()->size(); }

bool  PyDbShape::setSize(double s) { return PyWarnIfError(impObj()->setSize(s), L"AcDbShape::setSize"); }

double PyDbShape::rotation() const { return impObj()->rotation(); }

bool  PyDbShape::setRotation(double r) { return PyWarnIfError(impObj()->setRotation(r), L"AcDbShape::setRotation"); }

AcString PyDbShape::shapeName() const { AcString s; if(!PyWarnIfError(impObj()->name(s), L"AcDbShape::name")) return AcString(); return s; }
bool  PyDbShape::setShapeName(const AcString& n) { return PyWarnIfError(impObj()->setName(n.kwszPtr()), L"AcDbShape::setName"); }
double PyDbShape::widthFactor() const { return impObj()->widthFactor(); }
bool  PyDbShape::setWidthFactor(double v) { return PyWarnIfError(impObj()->setWidthFactor(v), L"AcDbShape::setWidthFactor"); }
double PyDbShape::oblique() const { return impObj()->oblique(); }
bool  PyDbShape::setOblique(double v) { return PyWarnIfError(impObj()->setOblique(v), L"AcDbShape::setOblique"); }
double PyDbShape::thickness() const { return impObj()->thickness(); }
bool  PyDbShape::setThickness(double v) { return PyWarnIfError(impObj()->setThickness(v), L"AcDbShape::setThickness"); }
AcGeVector3d PyDbShape::normal() const { return impObj()->normal(); }
bool  PyDbShape::setNormal(const AcGeVector3d& v) { return PyWarnIfError(impObj()->setNormal(v), L"AcDbShape::setNormal"); }
int PyDbShape::shapeNumber() const { return static_cast<int>(impObj()->shapeNumber()); }
bool  PyDbShape::setShapeNumber(int n) { return PyWarnIfError(impObj()->setShapeNumber(static_cast<Adesk::Int16>(n)), L"AcDbShape::setShapeNumber"); }
uint64_t PyDbShape::styleId() const { return static_cast<uint64_t>(impObj()->styleId().asOldId()); }

uint64_t PyDbShape::shapeIndex() const { return static_cast<uint64_t>(impObj()->shapeIndex().asOldId()); }
bool  PyDbShape::setStyleId(uint64_t id) { AcDbObjectId oid; oid.setFromOldId(static_cast<Adesk::UIntPtr>(id)); return PyWarnIfError(impObj()->setStyleId(oid), L"AcDbShape::setStyleId"); }
bool  PyDbShape::setShapeIndex(uint64_t id) { AcDbObjectId oid; oid.setFromOldId(static_cast<Adesk::UIntPtr>(id)); return PyWarnIfError(impObj()->setShapeIndex(oid), L"AcDbShape::setShapeIndex"); }

void bind_PyDbShape(pym& m) {
	py::class_<PyDbShape, PyDbEntity>(m, "PyDbShape")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&, double, double, double>(),
			py::arg("position"), py::arg("size"), py::arg("rotation") = 0.0, py::arg("widthFactor") = 0.0)
		.def(py::init<AcDbShape*>(), py::arg("object"))
		.def("position", &PyDbShape::position)
		.def("setPosition", &PyDbShape::setPosition, py::arg("p"))
		.def("size", &PyDbShape::size)
		.def("setSize", &PyDbShape::setSize, py::arg("s"))
		.def("shapeName", &PyDbShape::shapeName)
		.def("setShapeName", &PyDbShape::setShapeName, py::arg("n"))
		.def("rotation", &PyDbShape::rotation)
		.def("setRotation", &PyDbShape::setRotation, py::arg("r"))
		.def("widthFactor", &PyDbShape::widthFactor)
		.def("setWidthFactor", &PyDbShape::setWidthFactor, py::arg("v"))
		.def("oblique", &PyDbShape::oblique)
		.def("setOblique", &PyDbShape::setOblique, py::arg("v"))
		.def("thickness", &PyDbShape::thickness)
		.def("setThickness", &PyDbShape::setThickness, py::arg("t"))
		.def("normal", &PyDbShape::normal)
		.def("setNormal", &PyDbShape::setNormal, py::arg("n"))
		.def("shapeNumber", &PyDbShape::shapeNumber)
		.def("setShapeNumber", &PyDbShape::setShapeNumber, py::arg("n"))
		.def("styleId", &PyDbShape::styleId)
		.def("shapeIndex", &PyDbShape::shapeIndex)
		.def("setStyleId", &PyDbShape::setStyleId, py::arg("id"))
		.def("setShapeIndex", &PyDbShape::setShapeIndex, py::arg("id"))
		PyWrapper_BIND(DbShape)
		.def("__repr__", &PyDbShape::toString)
		;
}

