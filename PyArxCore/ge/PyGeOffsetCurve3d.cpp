#include "stdafx.h"
#include "PyGeOffsetCurve3d.h"
#include <format>
#include <string>

void bind_AcGeOffsetCurve3d(pybind11::module& m) {
	py::class_<AcGeOffsetCurve3d, AcGeCurve3d>(m, "AcGeOffsetCurve3d")
		.def(py::init<const AcGeCurve3d&, const AcGeVector3d&, double>(),
			py::arg("baseCurve"), py::arg("planeNormal"), py::arg("offsetDistance"))
		.def("normal", &AcGeOffsetCurve3d::normal)
		.def("offsetDistance", &AcGeOffsetCurve3d::offsetDistance)
		.def("paramDirection", &AcGeOffsetCurve3d::paramDirection)
		.def("transformation", &AcGeOffsetCurve3d::transformation)
		.def("setCurve", &AcGeOffsetCurve3d::setCurve, py::arg("baseCurve"))
		.def("setNormal", &AcGeOffsetCurve3d::setNormal, py::arg("normal"))
		.def("setOffsetDistance", &AcGeOffsetCurve3d::setOffsetDistance, py::arg("distance"))
		.def("__repr__", [](const AcGeOffsetCurve3d& c) {
			return std::format("AcGeOffsetCurve3d(offset={:.3f})", c.offsetDistance());
			});
}
