#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbEllipse.h"

PyWrapper_Cpp(DbEllipse)
AcString PyDbEllipse::toString() const { return AcString(std::format("AcDbEllipse[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbEllipse::center() const { return impObj() ? impObj()->center() : AcGePoint3d(); }

bool  PyDbEllipse::setCenter(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setCenter(p), L"AcDbEllipse::setCenter"); }

double PyDbEllipse::majorRadius() const { return impObj() ? impObj()->majorRadius() : 0.0; }

bool  PyDbEllipse::setMajorRadius(double r) { if (!impObj()) return false; return PyWarnIfError(impObj()->setMajorRadius(r), L"AcDbEllipse::setMajorRadius"); }

double PyDbEllipse::minorRadius() const { return impObj() ? impObj()->minorRadius() : 0.0; }

bool  PyDbEllipse::setMinorRadius(double r) { if (!impObj()) return false; return PyWarnIfError(impObj()->setMinorRadius(r), L"AcDbEllipse::setMinorRadius"); }

double PyDbEllipse::radiusRatio() const { return impObj() ? impObj()->radiusRatio() : 0.0; }

bool  PyDbEllipse::setRadiusRatio(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setRadiusRatio(v), L"AcDbEllipse::setRadiusRatio"); }

double PyDbEllipse::startAngle() const { return impObj() ? impObj()->startAngle() : 0.0; }

bool  PyDbEllipse::setStartAngle(double a) { if (!impObj()) return false; return PyWarnIfError(impObj()->setStartAngle(a), L"AcDbEllipse::setStartAngle"); }

double PyDbEllipse::endAngle() const { return impObj() ? impObj()->endAngle() : 0.0; }

bool  PyDbEllipse::setEndAngle(double a) { if (!impObj()) return false; return PyWarnIfError(impObj()->setEndAngle(a), L"AcDbEllipse::setEndAngle"); }

AcGeVector3d PyDbEllipse::normal() const { return impObj() ? impObj()->normal() : AcGeVector3d(); }
AcGeVector3d PyDbEllipse::majorAxis() const { return impObj() ? impObj()->majorAxis() : AcGeVector3d(); }
AcGeVector3d PyDbEllipse::minorAxis() const { return impObj() ? impObj()->minorAxis() : AcGeVector3d(); }
bool  PyDbEllipse::setStartParam(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setStartParam(v), L"AcDbEllipse::setStartParam"); }
bool  PyDbEllipse::setEndParam(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setEndParam(v), L"AcDbEllipse::setEndParam"); }
double PyDbEllipse::paramAtAngle(double angle) const { return impObj() ? impObj()->paramAtAngle(angle) : 0.0; }
double PyDbEllipse::angleAtParam(double param) const { return impObj() ? impObj()->angleAtParam(param) : 0.0; }
bool PyDbEllipse::isNull() const { return impObj() ? (impObj()->isNull() != Adesk::kFalse) : true; }

std::tuple<AcGePoint3d, AcGeVector3d, AcGeVector3d, double, double, double> PyDbEllipse::get() const {
	AcGePoint3d center;
	AcGeVector3d unitNormal, majorAxis;
	double radiusRatio = 0.0, startAngle = 0.0, endAngle = 0.0;
	if (!impObj()) return { AcGePoint3d(), AcGeVector3d(), AcGeVector3d(), 0.0, 0.0, 0.0 };
	if (!PyWarnIfError(impObj()->get(center, unitNormal, majorAxis, radiusRatio, startAngle, endAngle), L"AcDbEllipse::get")) {
		return { AcGePoint3d(), AcGeVector3d(), AcGeVector3d(), 0.0, 0.0, 0.0 };
	}
	return { center, unitNormal, majorAxis, radiusRatio, startAngle, endAngle };
}

bool PyDbEllipse::set(const AcGePoint3d& center, const AcGeVector3d& unitNormal, const AcGeVector3d& majorAxis, double radiusRatio, double startAngle, double endAngle) {
	if (!impObj()) return false;
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

