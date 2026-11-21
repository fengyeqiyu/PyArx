#include "stdafx.h"
#include "PyGeOffsetCurve2d.h"
#include <format>
#include <string>

void bind_AcGeOffsetCurve2d(pybind11::module& m) {
	py::class_<AcGeOffsetCurve2d, AcGeCurve2d>(m, "AcGeOffsetCurve2d")
		.def(py::init<const AcGeCurve2d&, double>(),
			py::arg("baseCurve"), py::arg("offsetDistance"))
		.def("offsetDistance", &AcGeOffsetCurve2d::offsetDistance)
		.def("paramDirection", &AcGeOffsetCurve2d::paramDirection)
		.def("transformation", &AcGeOffsetCurve2d::transformation)
		.def("setCurve", &AcGeOffsetCurve2d::setCurve, py::arg("baseCurve"))
		.def("setOffsetDistance", &AcGeOffsetCurve2d::setOffsetDistance, py::arg("distance"))
		.def("__repr__", [](const AcGeOffsetCurve2d& c) {
			return std::format("AcGeOffsetCurve2d(offset={:.3f})", c.offsetDistance());
			});
}
