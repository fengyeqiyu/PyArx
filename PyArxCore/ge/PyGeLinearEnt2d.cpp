#include "stdafx.h"
#include "PyGeLinearEnt2d.h"
#include <format>
#include <string>

void bind_AcGeLinearEnt2d(pybind11::module& m) {
	// Abstract base: no py::init.
	py::class_<AcGeLinearEnt2d, AcGeCurve2d>(m, "AcGeLinearEnt2d")
		.def("pointOnLine", &AcGeLinearEnt2d::pointOnLine)
		.def("direction", &AcGeLinearEnt2d::direction)
		.def("isParallelTo", &AcGeLinearEnt2d::isParallelTo,
			py::arg("line"), py::arg("tol") = AcGeContext::gTol)
		.def("isPerpendicularTo", &AcGeLinearEnt2d::isPerpendicularTo, py::arg("line"), py::arg("tol") = AcGeContext::gTol)
		.def("isColinearTo", &AcGeLinearEnt2d::isColinearTo, py::arg("line"), py::arg("tol") = AcGeContext::gTol)
		.def("intersectPoint", [](const AcGeLinearEnt2d& self, const AcGeLinearEnt2d& other, const AcGeTol& tol) { AcGePoint2d p; Adesk::Boolean ok = self.intersectWith(other, p, tol); return py::make_tuple(ok != Adesk::kFalse, p); }, py::arg("line"), py::arg("tol") = AcGeContext::gTol)
		.def("__repr__", [](const AcGeLinearEnt2d&) {
			return AcString(L"AcGeLinearEnt2d");
			});
}
