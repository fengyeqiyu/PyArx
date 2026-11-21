#include "stdafx.h"
#include "PyGeCylinder.h"
#include <format>
#include <string>

void bind_AcGeCylinder(pybind11::module& m) {
	py::class_<AcGeCylinder, AcGeSurface>(m, "AcGeCylinder")
		.def(py::init<>())
		.def(py::init<double, const AcGePoint3d&, const AcGeVector3d&>(),
			py::arg("radius"), py::arg("origin"), py::arg("axisOfSymmetry"))
		.def("radius", &AcGeCylinder::radius)
		.def("origin", &AcGeCylinder::origin)
		.def("axisOfSymmetry", &AcGeCylinder::axisOfSymmetry)
		.def("refAxis", &AcGeCylinder::refAxis)
		.def("isOuterNormal", &AcGeCylinder::isOuterNormal)
		.def("isClosed", &AcGeCylinder::isClosed, py::arg("tol") = AcGeContext::gTol)
		.def("heightAt", &AcGeCylinder::heightAt, py::arg("u"))
		.def("setRadius", &AcGeCylinder::setRadius, py::arg("radius"))
		.def("angles", [](const AcGeCylinder& c) { double a, b; c.getAngles(a, b); return py::make_tuple(a, b); })
		.def("__repr__", [](const AcGeCylinder& c) {
			AcGePoint3d o = c.origin();
			return std::format("AcGeCylinder(origin=({:.3f},{:.3f},{:.3f}), r={:.3f})",
				o.x, o.y, o.z, c.radius());
			});
}
