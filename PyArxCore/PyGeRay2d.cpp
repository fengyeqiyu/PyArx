#include "stdafx.h"
#include "PyGeRay2d.h"
#include <format>
#include <string>

void bind_AcGeRay2d(pybind11::module& m) {
	py::class_<AcGeRay2d, AcGeLinearEnt2d>(m, "AcGeRay2d")
		.def(py::init<>())
		.def(py::init<const AcGePoint2d&, const AcGeVector2d&>(),
			py::arg("pnt"), py::arg("vec"))
		.def(py::init<const AcGePoint2d&, const AcGePoint2d&>(),
			py::arg("pnt1"), py::arg("pnt2"))
		.def("pointOnLine", &AcGeRay2d::pointOnLine)
		.def("direction", &AcGeRay2d::direction)
		.def("set", py::overload_cast<const AcGePoint2d&, const AcGeVector2d&>(&AcGeRay2d::set), py::arg("pnt"), py::arg("vec"))
		.def("set", py::overload_cast<const AcGePoint2d&, const AcGePoint2d&>(&AcGeRay2d::set), py::arg("pnt1"), py::arg("pnt2"))
		.def("__repr__", [](const AcGeRay2d& r) {
			AcGePoint2d p = r.pointOnLine();
			AcGeVector2d d = r.direction();
			return std::format("AcGeRay2d(origin=({:.3f},{:.3f}), dir=({:.3f},{:.3f}))",
				p.x, p.y, d.x, d.y);
			});
}
