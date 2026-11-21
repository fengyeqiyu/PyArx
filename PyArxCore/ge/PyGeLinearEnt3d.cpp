#include "stdafx.h"
#include "PyGeLinearEnt3d.h"
#include <format>
#include <string>

void bind_AcGeLinearEnt3d(pybind11::module& m) {
	// Abstract base: no py::init.
	py::class_<AcGeLinearEnt3d, AcGeCurve3d>(m, "AcGeLinearEnt3d")
		.def("pointOnLine", &AcGeLinearEnt3d::pointOnLine)
		.def("direction", &AcGeLinearEnt3d::direction)
		.def("isParallelTo",
			py::overload_cast<const AcGeLinearEnt3d&, const AcGeTol&>(&AcGeLinearEnt3d::isParallelTo, py::const_),
			py::arg("line"), py::arg("tol") = AcGeContext::gTol)
		.def("isPerpendicularTo", py::overload_cast<const AcGeLinearEnt3d&, const AcGeTol&>(&AcGeLinearEnt3d::isPerpendicularTo, py::const_), py::arg("line"), py::arg("tol") = AcGeContext::gTol)
		.def("isColinearTo", &AcGeLinearEnt3d::isColinearTo, py::arg("line"), py::arg("tol") = AcGeContext::gTol)
		.def("intersectPoint", [](const AcGeLinearEnt3d& self, const AcGeLinearEnt3d& other, const AcGeTol& tol) { AcGePoint3d p; Adesk::Boolean ok = self.intersectWith(other, p, tol); return py::make_tuple(ok != Adesk::kFalse, p); }, py::arg("line"), py::arg("tol") = AcGeContext::gTol)
		.def("__repr__", [](const AcGeLinearEnt3d&) {
			return AcString(L"AcGeLinearEnt3d");
			});
}
