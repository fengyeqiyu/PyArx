#include "stdafx.h"
#include "PyGePosition3d.h"
#include <format>
#include <string>

void bind_AcGePosition3d(pybind11::module& m) {
	py::class_<AcGePosition3d>(m, "AcGePosition3d")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&>(), py::arg("pnt"))
		.def(py::init<double, double, double>(), py::arg("x"), py::arg("y"), py::arg("z"))
		.def(py::init<const AcGePosition3d&>(), py::arg("src"), "拷贝构造")
		.def("point3d", &AcGePosition3d::point3d)
		.def("set", py::overload_cast<const AcGePoint3d&>(&AcGePosition3d::set))
		.def("set", py::overload_cast<double, double, double>(&AcGePosition3d::set),
			py::arg("x"), py::arg("y"), py::arg("z"))
		.def("__repr__", [](const AcGePosition3d& p) {
			AcGePoint3d pt = p.point3d();
			return std::format("AcGePosition3d({:.3f},{:.3f},{:.3f})", pt.x, pt.y, pt.z);
			});
}
