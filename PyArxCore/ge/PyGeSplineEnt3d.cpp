#include "stdafx.h"
#include "PyGeSplineEnt3d.h"
#include <gekvec.h>
#include <format>
#include <string>

void bind_AcGeSplineEnt3d(pybind11::module& m) {
	// Abstract base: no py::init.
	py::class_<AcGeSplineEnt3d, AcGeCurve3d>(m, "AcGeSplineEnt3d")
		.def("degree", &AcGeSplineEnt3d::degree)
		.def("order", &AcGeSplineEnt3d::order)
		.def("numKnots", &AcGeSplineEnt3d::numKnots)
		.def("numControlPoints", &AcGeSplineEnt3d::numControlPoints)
		.def("startParam", &AcGeSplineEnt3d::startParam)
		.def("endParam", &AcGeSplineEnt3d::endParam)
		.def("startPoint", &AcGeSplineEnt3d::startPoint)
		.def("endPoint", &AcGeSplineEnt3d::endPoint)
		.def("knotAt", &AcGeSplineEnt3d::knotAt, py::arg("index"))
		.def("controlPointAt", &AcGeSplineEnt3d::controlPointAt, py::arg("index"))
		.def("isRational", &AcGeSplineEnt3d::isRational)
		.def("hasFitData", &AcGeSplineEnt3d::hasFitData)
		.def("continuityAtKnot", &AcGeSplineEnt3d::continuityAtKnot, py::arg("index"), py::arg("tol") = AcGeContext::gTol)
		.def("setKnotAt", &AcGeSplineEnt3d::setKnotAt, py::arg("index"), py::arg("value"))
		.def("setControlPointAt", &AcGeSplineEnt3d::setControlPointAt, py::arg("index"), py::arg("pnt"))
		.def("knots", [](const AcGeSplineEnt3d& self) -> AcGeKnotVector {
			return self.knots();
		})
		.def("__repr__", [](const AcGeSplineEnt3d& s) {
			return std::format("AcGeSplineEnt3d(degree={})", s.degree());
			});
}
