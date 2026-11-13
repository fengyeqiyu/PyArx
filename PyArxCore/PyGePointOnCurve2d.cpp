#include "stdafx.h"
#include "PyGePointOnCurve2d.h"
#include <format>
#include <string>

void bind_AcGePointOnCurve2d(pybind11::module& m) {
	py::class_<AcGePointOnCurve2d>(m, "AcGePointOnCurve2d")
		.def(py::init<>())
		.def(py::init<const AcGeCurve2d&>(), py::arg("curve"))
		.def(py::init<const AcGeCurve2d&, double>(),
			py::arg("curve"), py::arg("param"))
		.def("parameter", &AcGePointOnCurve2d::parameter)
		.def("point2d", &AcGePointOnCurve2d::point2d)
		.def("point", py::overload_cast<>(&AcGePointOnCurve2d::point, py::const_))
		.def("point", py::overload_cast<double>(&AcGePointOnCurve2d::point),
			py::arg("param"))
		.def("deriv", py::overload_cast<int>(&AcGePointOnCurve2d::deriv, py::const_),
			py::arg("order"))
		// isSingular 未导出，留骨架
		.def("setCurve", &AcGePointOnCurve2d::setCurve)
		.def("setParameter", &AcGePointOnCurve2d::setParameter, py::arg("param"))
		// NOTE: curvature(double& res) output-param deferred; raw curve() pointer skipped.
		.def("deriv", py::overload_cast<int, double>(&AcGePointOnCurve2d::deriv), py::arg("order"), py::arg("param"))
		// isSingular 未导出，留骨架
		// curvature(double& res) 未导出，留骨架
		.def("__repr__", [](const AcGePointOnCurve2d& p) {
			AcGePoint2d pt = p.point2d();
			return std::format("AcGePointOnCurve2d(param={:.3f}, p=({:.3f},{:.3f}))",
				p.parameter(), pt.x, pt.y);
			});
}
