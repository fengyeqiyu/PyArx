#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbEllipse.h"

PyWrapper_Cpp(DbEllipse)
AcString PyDbEllipse::toString() const { return AcString(std::format("AcDbEllipse[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbEllipse::center() const { return impObj()->center(); }

bool  PyDbEllipse::setCenter(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setCenter(p), L"AcDbEllipse::setCenter"); }

double PyDbEllipse::majorRadius() const { return impObj()->majorRadius(); }

bool  PyDbEllipse::setMajorRadius(double r) { return PyWarnIfError(impObj()->setMajorRadius(r), L"AcDbEllipse::setMajorRadius"); }

double PyDbEllipse::minorRadius() const { return impObj()->minorRadius(); }

bool  PyDbEllipse::setMinorRadius(double r) { return PyWarnIfError(impObj()->setMinorRadius(r), L"AcDbEllipse::setMinorRadius"); }

double PyDbEllipse::radiusRatio() const { return impObj()->radiusRatio(); }

bool  PyDbEllipse::setRadiusRatio(double v) { return PyWarnIfError(impObj()->setRadiusRatio(v), L"AcDbEllipse::setRadiusRatio"); }

double PyDbEllipse::startAngle() const { return impObj()->startAngle(); }

bool  PyDbEllipse::setStartAngle(double a) { return PyWarnIfError(impObj()->setStartAngle(a), L"AcDbEllipse::setStartAngle"); }

double PyDbEllipse::endAngle() const { return impObj()->endAngle(); }

bool  PyDbEllipse::setEndAngle(double a) { return PyWarnIfError(impObj()->setEndAngle(a), L"AcDbEllipse::setEndAngle"); }

AcGeVector3d PyDbEllipse::normal() const { return impObj()->normal(); }
AcGeVector3d PyDbEllipse::majorAxis() const { return impObj()->majorAxis(); }
AcGeVector3d PyDbEllipse::minorAxis() const { return impObj()->minorAxis(); }
bool  PyDbEllipse::setStartParam(double v) { return PyWarnIfError(impObj()->setStartParam(v), L"AcDbEllipse::setStartParam"); }
bool  PyDbEllipse::setEndParam(double v) { return PyWarnIfError(impObj()->setEndParam(v), L"AcDbEllipse::setEndParam"); }
double PyDbEllipse::paramAtAngle(double angle) const { return impObj()->paramAtAngle(angle); }
double PyDbEllipse::angleAtParam(double param) const { return impObj()->angleAtParam(param); }
bool PyDbEllipse::isNull() const { return impObj()->isNull() != Adesk::kFalse; }

std::tuple<AcGePoint3d, AcGeVector3d, AcGeVector3d, double, double, double> PyDbEllipse::get() const {
	AcGePoint3d center;
	AcGeVector3d unitNormal, majorAxis;
	double radiusRatio = 0.0, startAngle = 0.0, endAngle = 0.0;
	if (!PyWarnIfError(impObj()->get(center, unitNormal, majorAxis, radiusRatio, startAngle, endAngle), L"AcDbEllipse::get")) {
		return { AcGePoint3d(), AcGeVector3d(), AcGeVector3d(), 0.0, 0.0, 0.0 };
	}
	return { center, unitNormal, majorAxis, radiusRatio, startAngle, endAngle };
}

bool PyDbEllipse::set(const AcGePoint3d& center, const AcGeVector3d& unitNormal, const AcGeVector3d& majorAxis, double radiusRatio, double startAngle, double endAngle) {
	return PyWarnIfError(impObj()->set(center, unitNormal, majorAxis, radiusRatio, startAngle, endAngle), L"AcDbEllipse::set");
}

void bind_PyDbEllipse(pym& m) {
	py::class_<PyDbEllipse, PyDbCurve>(m, "PyDbEllipse")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&, const AcGeVector3d&, const AcGeVector3d&, double, double, double>(),
			py::arg("center"), py::arg("unitNormal"), py::arg("majorAxis"), py::arg("radiusRatio"),
			py::arg("startAngle") = 0.0, py::arg("endAngle") = 6.28318530717958647692)
		.def(py::init<AcDbEllipse*>(), py::arg("object"))
		.def("center", &PyDbEllipse::center)
		.def("setCenter", &PyDbEllipse::setCenter, py::arg("p"))
		.def("normal", &PyDbEllipse::normal)
		.def("majorAxis", &PyDbEllipse::majorAxis)
		.def("minorAxis", &PyDbEllipse::minorAxis)
		.def("majorRadius", &PyDbEllipse::majorRadius)
		.def("setMajorRadius", &PyDbEllipse::setMajorRadius, py::arg("r"))
		.def("minorRadius", &PyDbEllipse::minorRadius)
		.def("setMinorRadius", &PyDbEllipse::setMinorRadius, py::arg("r"))
		.def("radiusRatio", &PyDbEllipse::radiusRatio)
		.def("setRadiusRatio", &PyDbEllipse::setRadiusRatio, py::arg("v"))
		.def("startAngle", &PyDbEllipse::startAngle)
		.def("setStartAngle", &PyDbEllipse::setStartAngle, py::arg("a"))
		.def("endAngle", &PyDbEllipse::endAngle)
		.def("setEndAngle", &PyDbEllipse::setEndAngle, py::arg("a"))
		.def("setStartParam", &PyDbEllipse::setStartParam, py::arg("v"))
		.def("setEndParam", &PyDbEllipse::setEndParam, py::arg("v"))
		.def("paramAtAngle", &PyDbEllipse::paramAtAngle, py::arg("angle"))
		.def("angleAtParam", &PyDbEllipse::angleAtParam, py::arg("param"))
		.def("isNull", &PyDbEllipse::isNull)
		.def("get", &PyDbEllipse::get)
		.def("set", &PyDbEllipse::set, py::arg("center"), py::arg("unitNormal"), py::arg("majorAxis"), py::arg("radiusRatio"), py::arg("startAngle") = 0.0, py::arg("endAngle") = 6.28318530717958647692)
		PyWrapper_BIND(DbEllipse)
		.def("__repr__", &PyDbEllipse::toString)
		;
}

