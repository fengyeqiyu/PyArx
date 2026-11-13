#include "stdafx.h"
#include "PyGePlanarEnt.h"
#include <format>
#include <string>

void bind_AcGePlanarEnt(pybind11::module& m) {
	// Abstract base: no py::init.
	py::class_<AcGePlanarEnt, AcGeSurface>(m, "AcGePlanarEnt")
		.def("pointOnPlane", &AcGePlanarEnt::pointOnPlane)
		.def("normal", &AcGePlanarEnt::normal)
		.def("isParallelToLine", py::overload_cast<const AcGeLinearEnt3d&, const AcGeTol&>(&AcGePlanarEnt::isParallelTo, py::const_), py::arg("line"), py::arg("tol") = AcGeContext::gTol)
		.def("isParallelToPlane", py::overload_cast<const AcGePlanarEnt&, const AcGeTol&>(&AcGePlanarEnt::isParallelTo, py::const_), py::arg("other"), py::arg("tol") = AcGeContext::gTol)
		.def("isPerpendicularToLine", py::overload_cast<const AcGeLinearEnt3d&, const AcGeTol&>(&AcGePlanarEnt::isPerpendicularTo, py::const_), py::arg("line"), py::arg("tol") = AcGeContext::gTol)
		.def("isCoplanarTo", &AcGePlanarEnt::isCoplanarTo, py::arg("other"), py::arg("tol") = AcGeContext::gTol)
		.def("coefficients", [](const AcGePlanarEnt& self) { double a, b, c, d; self.getCoefficients(a, b, c, d); return py::make_tuple(a, b, c, d); })
			.def("getCoordSystem", [](const AcGePlanarEnt& self) -> py::tuple {
				AcGePoint3d origin; AcGeVector3d axis1, axis2;
				self.getCoordSystem(origin, axis1, axis2);
				return py::make_tuple(origin, axis1, axis2);
				})
		.def("__repr__", [](const AcGePlanarEnt&) {
			return AcString(L"AcGePlanarEnt");
			});
}
