#include "stdafx.h"
#include "PyGeSplineEnt2d.h"
#include <gekvec.h>
#include <format>
#include <string>

void bind_AcGeSplineEnt2d(pybind11::module& m) {
	// Abstract base: no py::init.
	py::class_<AcGeSplineEnt2d, AcGeCurve2d>(m, "AcGeSplineEnt2d")
		.def("degree", &AcGeSplineEnt2d::degree)
		.def("order", &AcGeSplineEnt2d::order)
		.def("numKnots", &AcGeSplineEnt2d::numKnots)
		.def("numControlPoints", &AcGeSplineEnt2d::numControlPoints)
		.def("startParam", &AcGeSplineEnt2d::startParam)
		.def("endParam", &AcGeSplineEnt2d::endParam)
		.def("startPoint", &AcGeSplineEnt2d::startPoint)
		.def("endPoint", &AcGeSplineEnt2d::endPoint)
		.def("knotAt", &AcGeSplineEnt2d::knotAt, py::arg("index"))
		.def("controlPointAt", &AcGeSplineEnt2d::controlPointAt, py::arg("index"))
		.def("isRational", &AcGeSplineEnt2d::isRational)
		.def("hasFitData", &AcGeSplineEnt2d::hasFitData)
		.def("continuityAtKnot", &AcGeSplineEnt2d::continuityAtKnot, py::arg("index"), py::arg("tol") = AcGeContext::gTol)
		.def("setKnotAt", &AcGeSplineEnt2d::setKnotAt, py::arg("index"), py::arg("value"))
		.def("setControlPointAt", &AcGeSplineEnt2d::setControlPointAt, py::arg("index"), py::arg("pnt"))
		.def("knots", [](const AcGeSplineEnt2d& self) -> AcGeKnotVector {
			return self.knots();
		})
		.def("__repr__", [](const AcGeSplineEnt2d& s) {
			return std::format("AcGeSplineEnt2d(degree={})", s.degree());
			});
}
