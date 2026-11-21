#include "stdafx.h"
#include "PyGePointOnCurve3d.h"
#include <format>
#include <string>

void bind_AcGePointOnCurve3d(pybind11::module& m) {
	py::class_<AcGePointOnCurve3d>(m, "AcGePointOnCurve3d")
		.def(py::init<>())
		.def(py::init<const AcGeCurve3d&>(), py::arg("curve"))
		.def(py::init<const AcGeCurve3d&, double>(),
			py::arg("curve"), py::arg("param"))
		.def("parameter", &AcGePointOnCurve3d::parameter)
		.def("point3d", &AcGePointOnCurve3d::point3d)
		.def("point", py::overload_cast<>(&AcGePointOnCurve3d::point, py::const_))
		.def("point", py::overload_cast<double>(&AcGePointOnCurve3d::point),
			py::arg("param"))
		.def("deriv", py::overload_cast<int>(&AcGePointOnCurve3d::deriv, py::const_),
			py::arg("order"))
		// isSingular 未导出，留骨架
		.def("setCurve", &AcGePointOnCurve3d::setCurve)
		.def("setParameter", &AcGePointOnCurve3d::setParameter, py::arg("param"))
		.def("deriv", py::overload_cast<int, double>(&AcGePointOnCurve3d::deriv), py::arg("order"), py::arg("param"))
		// isSingular 未导出，留骨架
		// curvature(double& res) 未导出，留骨架
		.def("__repr__", [](const AcGePointOnCurve3d& p) {
			AcGePoint3d pt = p.point3d();
			return std::format("AcGePointOnCurve3d(param={:.3f}, p=({:.3f},{:.3f},{:.3f}))",
				p.parameter(), pt.x, pt.y, pt.z);
			});
}
