#include "stdafx.h"
#include "PyGeTorus.h"
#include <format>
#include <string>

void bind_AcGeTorus(pybind11::module& m) {
	py::class_<AcGeTorus, AcGeSurface>(m, "AcGeTorus")
		.def(py::init<>())
		.def(py::init<double, double, const AcGePoint3d&, const AcGeVector3d&>(),
			py::arg("majorRadius"), py::arg("minorRadius"),
			py::arg("origin"), py::arg("axisOfSymmetry"))
		.def("majorRadius", &AcGeTorus::majorRadius)
		.def("minorRadius", &AcGeTorus::minorRadius)
		.def("center", &AcGeTorus::center)
		.def("isDoughnut", &AcGeTorus::isDoughnut)
		.def("axisOfSymmetry", &AcGeTorus::axisOfSymmetry)
		.def("refAxis", &AcGeTorus::refAxis)
		.def("isOuterNormal", &AcGeTorus::isOuterNormal)
		.def("isLemon", &AcGeTorus::isLemon)
		.def("isApple", &AcGeTorus::isApple)
		.def("isVortex", &AcGeTorus::isVortex)
		.def("isDegenerate", &AcGeTorus::isDegenerate)
		.def("isHollow", &AcGeTorus::isHollow)
		.def("anglesInU", [](const AcGeTorus& t) { double a, b; t.getAnglesInU(a, b); return py::make_tuple(a, b); })
		.def("anglesInV", [](const AcGeTorus& t) { double a, b; t.getAnglesInV(a, b); return py::make_tuple(a, b); })
		.def("setMajorRadius", &AcGeTorus::setMajorRadius, py::arg("radius"))
		.def("setMinorRadius", &AcGeTorus::setMinorRadius, py::arg("radius"))
		.def("__repr__", [](const AcGeTorus& t) {
			AcGePoint3d c = t.center();
			return std::format("AcGeTorus(center=({:.3f},{:.3f},{:.3f}), majR={:.3f}, minR={:.3f})",
				c.x, c.y, c.z, t.majorRadius(), t.minorRadius());
			});
}
