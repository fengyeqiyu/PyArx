#include "stdafx.h"
#include "PyGeCone.h"
#include <format>
#include <string>

void bind_AcGeCone(pybind11::module& m) {
	py::class_<AcGeCone, AcGeSurface>(m, "AcGeCone")
		.def(py::init<>())
		.def(py::init<double, double, const AcGePoint3d&, double, const AcGeVector3d&>(),
			py::arg("cosineAngle"), py::arg("sineAngle"),
			py::arg("baseOrigin"), py::arg("baseRadius"), py::arg("axisOfSymmetry"))
		.def("baseRadius", &AcGeCone::baseRadius)
		.def("baseCenter", &AcGeCone::baseCenter)
		.def("apex", &AcGeCone::apex)
		.def("halfAngle", &AcGeCone::halfAngle)
		.def("axisOfSymmetry", &AcGeCone::axisOfSymmetry)
		.def("refAxis", &AcGeCone::refAxis)
		.def("isClosed", &AcGeCone::isClosed, py::arg("tol") = AcGeContext::gTol)
		.def("isOuterNormal", &AcGeCone::isOuterNormal)
		.def("setBaseRadius", &AcGeCone::setBaseRadius, py::arg("radius"))
		.def("angles", [](const AcGeCone& c) { double a, b; c.getAngles(a, b); return py::make_tuple(a, b); })
		.def("__repr__", [](const AcGeCone& c) {
			AcGePoint3d o = c.baseCenter();
			return std::format("AcGeCone(baseCenter=({:.3f},{:.3f},{:.3f}), baseR={:.3f})",
				o.x, o.y, o.z, c.baseRadius());
			});
}
