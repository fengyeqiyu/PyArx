#include "stdafx.h"
#include "PyGePosition2d.h"
#include <format>
#include <string>

void bind_AcGePosition2d(pybind11::module& m) {
	py::class_<AcGePosition2d>(m, "AcGePosition2d")
		.def(py::init<>())
		.def(py::init<const AcGePoint2d&>(), py::arg("pnt"))
		.def(py::init<double, double>(), py::arg("x"), py::arg("y"))
		.def(py::init<const AcGePosition2d&>(), py::arg("src"), "拷贝构造")
		.def("point2d", &AcGePosition2d::point2d)
		.def("set", py::overload_cast<const AcGePoint2d&>(&AcGePosition2d::set))
		.def("set", py::overload_cast<double, double>(&AcGePosition2d::set),
			py::arg("x"), py::arg("y"))
		.def("__repr__", [](const AcGePosition2d& p) {
			AcGePoint2d pt = p.point2d();
			return std::format("AcGePosition2d({:.3f},{:.3f})", pt.x, pt.y);
			});
}
