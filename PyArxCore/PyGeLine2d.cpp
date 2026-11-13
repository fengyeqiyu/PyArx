#include "stdafx.h"
#include "PyGeLine2d.h"
#include <format>
#include <string>

void bind_AcGeLine2d(pybind11::module& m) {
	py::class_<AcGeLine2d, AcGeLinearEnt2d>(m, "AcGeLine2d")
		.def(py::init<>())
		.def(py::init<const AcGeLine2d&>(), py::arg("src"))
		.def(py::init<const AcGePoint2d&, const AcGeVector2d&>(),
			py::arg("pnt"), py::arg("vec"))
		.def(py::init<const AcGePoint2d&, const AcGePoint2d&>(),
			py::arg("pnt1"), py::arg("pnt2"))
		.def_readonly_static("kXAxis", &AcGeLine2d::kXAxis)
		.def_readonly_static("kYAxis", &AcGeLine2d::kYAxis)
		.def("set", py::overload_cast<const AcGePoint2d&, const AcGeVector2d&>(&AcGeLine2d::set),
			py::arg("pnt"), py::arg("vec"))
		.def("set", py::overload_cast<const AcGePoint2d&, const AcGePoint2d&>(&AcGeLine2d::set), py::arg("pnt1"), py::arg("pnt2"))
		.def("__repr__", [](const AcGeLine2d& l) {
			AcGePoint2d p = l.pointOnLine();
			AcGeVector2d d = l.direction();
			return std::format("AcGeLine2d(point=({:.3f},{:.3f}), dir=({:.3f},{:.3f}))",
				p.x, p.y, d.x, d.y);
			});
}
