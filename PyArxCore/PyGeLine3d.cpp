#include "stdafx.h"
#include "PyGeLine3d.h"
#include <format>
#include <string>

void bind_AcGeLine3d(pybind11::module& m) {
	py::class_<AcGeLine3d, AcGeLinearEnt3d>(m, "AcGeLine3d")
		.def(py::init<>())
		.def(py::init<const AcGeLine3d&>(), py::arg("src"))
		.def(py::init<const AcGePoint3d&, const AcGeVector3d&>(),
			py::arg("pnt"), py::arg("vec"))
		.def(py::init<const AcGePoint3d&, const AcGePoint3d&>(),
			py::arg("pnt1"), py::arg("pnt2"))
		.def_readonly_static("kXAxis", &AcGeLine3d::kXAxis)
		.def_readonly_static("kYAxis", &AcGeLine3d::kYAxis)
		.def_readonly_static("kZAxis", &AcGeLine3d::kZAxis)
		.def("set", py::overload_cast<const AcGePoint3d&, const AcGeVector3d&>(&AcGeLine3d::set),
			py::arg("pnt"), py::arg("vec"))
		.def("set", py::overload_cast<const AcGePoint3d&, const AcGePoint3d&>(&AcGeLine3d::set), py::arg("pnt1"), py::arg("pnt2"))
		.def("__repr__", [](const AcGeLine3d& l) {
			AcGePoint3d p = l.pointOnLine();
			AcGeVector3d d = l.direction();
			return std::format("AcGeLine3d(point=({:.3f},{:.3f},{:.3f}), dir=({:.3f},{:.3f},{:.3f}))",
				p.x, p.y, p.z, d.x, d.y, d.z);
			});
}
