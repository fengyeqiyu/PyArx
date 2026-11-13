#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbHelix.h"

PyWrapper_Cpp(DbHelix)
AcString PyDbHelix::toString() const { return AcString(std::format("AcDbHelix[wrapper]").c_str(), AcString::Utf8); }
double PyDbHelix::height() const { return impObj()->height(); }

bool  PyDbHelix::setHeight(double h) { return PyWarnIfError(impObj()->setHeight(h), L"AcDbHelix::setHeight"); }

double PyDbHelix::baseRadius() const { return impObj()->baseRadius(); }

bool  PyDbHelix::setBaseRadius(double r) { return PyWarnIfError(impObj()->setBaseRadius(r), L"AcDbHelix::setBaseRadius"); }

double PyDbHelix::topRadius() const { return impObj()->topRadius(); }

bool  PyDbHelix::setTopRadius(double r) { return PyWarnIfError(impObj()->setTopRadius(r), L"AcDbHelix::setTopRadius"); }

double PyDbHelix::turns() const { return impObj()->turns(); }

bool  PyDbHelix::setTurns(double t) { return PyWarnIfError(impObj()->setTurns(t), L"AcDbHelix::setTurns"); }

AcGePoint3d PyDbHelix::startPoint() const { return impObj()->startPoint(); }

bool  PyDbHelix::setStartPoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setStartPoint(p), L"AcDbHelix::setStartPoint"); }

bool  PyDbHelix::createHelix() { return PyWarnIfError(impObj()->createHelix(), L"AcDbHelix::createHelix"); }
AcGePoint3d PyDbHelix::axisPoint() const { return impObj()->axisPoint(); }
bool  PyDbHelix::setAxisPoint(const AcGePoint3d& p, bool bMoveStartPoint) { return PyWarnIfError(impObj()->setAxisPoint(p, bMoveStartPoint), L"AcDbHelix::setAxisPoint"); }
AcGeVector3d PyDbHelix::axisVector() const { return impObj()->axisVector(); }
bool  PyDbHelix::setAxisVector(const AcGeVector3d& v) { return PyWarnIfError(impObj()->setAxisVector(v), L"AcDbHelix::setAxisVector"); }
double PyDbHelix::turnHeight() const { return impObj()->turnHeight(); }
bool  PyDbHelix::setTurnHeight(double v) { return PyWarnIfError(impObj()->setTurnHeight(v), L"AcDbHelix::setTurnHeight"); }
bool PyDbHelix::twist() const { return impObj()->twist() != Adesk::kFalse; }
bool  PyDbHelix::setTwist(bool b) { return PyWarnIfError(impObj()->setTwist(b ? Adesk::kTrue : Adesk::kFalse), L"AcDbHelix::setTwist"); }
int PyDbHelix::constrain() const { return static_cast<int>(impObj()->constrain()); }
bool  PyDbHelix::setConstrain(int c) { return PyWarnIfError(impObj()->setConstrain(static_cast<AcDbHelix::ConstrainType>(c)), L"AcDbHelix::setConstrain"); }
double PyDbHelix::turnSlope() const { return impObj()->turnSlope(); }
double PyDbHelix::totalLength() const { return impObj()->totalLength(); }

void bind_PyDbHelix(pym& m) {
	py::class_<PyDbHelix, PyDbSpline>(m, "PyDbHelix")
		.def(py::init<>())
		.def(py::init<AcDbHelix*>(), py::arg("object"))
		.def("height", &PyDbHelix::height)
		.def("setHeight", &PyDbHelix::setHeight, py::arg("h"))
		.def("baseRadius", &PyDbHelix::baseRadius)
		.def("setBaseRadius", &PyDbHelix::setBaseRadius, py::arg("r"))
		.def("topRadius", &PyDbHelix::topRadius)
		.def("setTopRadius", &PyDbHelix::setTopRadius, py::arg("r"))
		.def("turns", &PyDbHelix::turns)
		.def("setTurns", &PyDbHelix::setTurns, py::arg("t"))
		.def("startPoint", &PyDbHelix::startPoint)
		.def("setStartPoint", &PyDbHelix::setStartPoint, py::arg("p"))
		.def("createHelix", &PyDbHelix::createHelix)
		.def("axisPoint", &PyDbHelix::axisPoint)
		.def("setAxisPoint", &PyDbHelix::setAxisPoint, py::arg("p"), py::arg("bMoveStartPoint") = true)
		.def("axisVector", &PyDbHelix::axisVector)
		.def("setAxisVector", &PyDbHelix::setAxisVector, py::arg("v"))
		.def("turnHeight", &PyDbHelix::turnHeight)
		.def("setTurnHeight", &PyDbHelix::setTurnHeight, py::arg("v"))
		.def("twist", &PyDbHelix::twist)
		.def("setTwist", &PyDbHelix::setTwist, py::arg("b"))
		.def("constrain", &PyDbHelix::constrain)
		.def("setConstrain", &PyDbHelix::setConstrain, py::arg("c"))
		.def("turnSlope", &PyDbHelix::turnSlope)
		.def("totalLength", &PyDbHelix::totalLength)
		PyWrapper_BIND(DbHelix)
		.def("__repr__", &PyDbHelix::toString)
		;
}

