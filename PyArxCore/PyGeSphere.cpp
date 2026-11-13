#include "stdafx.h"
#include "PyGeSphere.h"
#include <format>
#include <string>

void bind_AcGeSphere(pybind11::module& m) {
	py::class_<AcGeSphere, AcGeSurface>(m, "AcGeSphere")
		.def(py::init<>())
		.def(py::init<double, const AcGePoint3d&>(),
			py::arg("radius"), py::arg("center"))
		.def("radius", &AcGeSphere::radius)
		.def("center", &AcGeSphere::center)
		.def("northPole", &AcGeSphere::northPole)
		.def("southPole", &AcGeSphere::southPole)
		.def("isClosed", &AcGeSphere::isClosed,
			py::arg("tol") = AcGeContext::gTol)
		.def("northAxis", &AcGeSphere::northAxis)
		.def("refAxis", &AcGeSphere::refAxis)
		.def("isOuterNormal", &AcGeSphere::isOuterNormal)
		.def("setRadius", &AcGeSphere::setRadius, py::arg("radius"))
		.def("set", py::overload_cast<double, const AcGePoint3d&>(&AcGeSphere::set), py::arg("radius"), py::arg("center"))
		.def("anglesInU", [](const AcGeSphere& s) { double a, b; s.getAnglesInU(a, b); return py::make_tuple(a, b); })
		.def("anglesInV", [](const AcGeSphere& s) { double a, b; s.getAnglesInV(a, b); return py::make_tuple(a, b); })
		.def("__repr__", [](const AcGeSphere& s) {
			AcGePoint3d c = s.center();
			return std::format("AcGeSphere(center=({:.3f},{:.3f},{:.3f}), r={:.3f})",
				c.x, c.y, c.z, s.radius());
			});
}
