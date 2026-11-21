#include "stdafx.h"
#include "PyGeSurface.h"
#include <format>
#include <string>

void bind_AcGeSurface(pybind11::module& m) {
	// Abstract base: no py::init.
	py::class_<AcGeSurface, AcGeEntity3d>(m, "AcGeSurface")
		.def("isOn",
			py::overload_cast<const AcGePoint3d&, const AcGeTol&>(&AcGeSurface::isOn, py::const_),
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("closestPointTo", &AcGeSurface::closestPointTo,
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("distanceTo", &AcGeSurface::distanceTo,
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("paramOf", &AcGeSurface::paramOf,
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("isClosedInU", &AcGeSurface::isClosedInU,
			py::arg("tol") = AcGeContext::gTol)
		.def("isClosedInV", &AcGeSurface::isClosedInV,
			py::arg("tol") = AcGeContext::gTol)
		.def("evalPoint", py::overload_cast<const AcGePoint2d&>(&AcGeSurface::evalPoint, py::const_), py::arg("param"))
		.def("isNormalReversed", &AcGeSurface::isNormalReversed)
		.def("reverseNormal", &AcGeSurface::reverseNormal)
		.def("getEnvelope", [](const AcGeSurface& self) { AcGeInterval x, y; self.getEnvelope(x, y); return py::make_tuple(py::make_tuple(x.lowerBound(), x.upperBound()), py::make_tuple(y.lowerBound(), y.upperBound())); })
		.def("__repr__", [](const AcGeSurface&) {
			return AcString(L"AcGeSurface");
			});
}
