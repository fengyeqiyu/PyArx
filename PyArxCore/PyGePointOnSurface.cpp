#include "stdafx.h"
#include "PyGePointOnSurface.h"
#include <format>
#include <string>

void bind_AcGePointOnSurface(pybind11::module& m) {
	py::class_<AcGePointOnSurface>(m, "AcGePointOnSurface")
		.def(py::init<>())
		.def(py::init<const AcGeSurface&>(), py::arg("surface"))
		.def(py::init<const AcGeSurface&, const AcGePoint2d&>(),
			py::arg("surface"), py::arg("param"))
		.def("parameter", &AcGePointOnSurface::parameter)
		.def("point3d", &AcGePointOnSurface::point3d)
		.def("point", py::overload_cast<>(&AcGePointOnSurface::point, py::const_))
		.def("point", py::overload_cast<const AcGePoint2d&>(&AcGePointOnSurface::point),
			py::arg("param"))
		.def("normal", py::overload_cast<>(&AcGePointOnSurface::normal, py::const_))
		.def("normal", py::overload_cast<const AcGePoint2d&>(&AcGePointOnSurface::normal),
			py::arg("param"))
		.def("setSurface", &AcGePointOnSurface::setSurface)
		.def("setParameter", &AcGePointOnSurface::setParameter, py::arg("param"))
		// Derivatives / mixed partial / tangent mapping: const overloads bound explicitly below.
		.def("uDeriv", py::overload_cast<int>(&AcGePointOnSurface::uDeriv, py::const_), py::arg("order"))
		.def("vDeriv", py::overload_cast<int>(&AcGePointOnSurface::vDeriv, py::const_), py::arg("order"))
		.def("mixedPartial", py::overload_cast<>(&AcGePointOnSurface::mixedPartial, py::const_))
		// tangentVector / inverseTangentVector are multi-overload; bind the const
		// no-param overloads explicitly so pybind11 can resolve the function pointer.
		.def("tangentVector", py::overload_cast<const AcGeVector2d&>(&AcGePointOnSurface::tangentVector, py::const_), py::arg("vec"))
		.def("inverseTangentVector", py::overload_cast<const AcGeVector3d&>(&AcGePointOnSurface::inverseTangentVector, py::const_), py::arg("vec"))
		.def("__repr__", [](const AcGePointOnSurface& p) {
			AcGePoint2d prm = p.parameter();
			return std::format("AcGePointOnSurface(u={:.3f},v={:.3f})", prm.x, prm.y);
			});
}
