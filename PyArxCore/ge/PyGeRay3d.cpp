#include "stdafx.h"
#include "PyGeRay3d.h"
#include <format>
#include <string>

void bind_AcGeRay3d(pybind11::module& m) {
	py::class_<AcGeRay3d, AcGeLinearEnt3d>(m, "AcGeRay3d")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&, const AcGeVector3d&>(),
			py::arg("pnt"), py::arg("vec"))
		.def(py::init<const AcGePoint3d&, const AcGePoint3d&>(),
			py::arg("pnt1"), py::arg("pnt2"))
		.def("pointOnLine", &AcGeRay3d::pointOnLine)
		.def("direction", &AcGeRay3d::direction)
		.def("set", py::overload_cast<const AcGePoint3d&, const AcGeVector3d&>(&AcGeRay3d::set), py::arg("pnt"), py::arg("vec"))
		.def("set", py::overload_cast<const AcGePoint3d&, const AcGePoint3d&>(&AcGeRay3d::set), py::arg("pnt1"), py::arg("pnt2"))
		.def("__repr__", [](const AcGeRay3d& r) {
			AcGePoint3d p = r.pointOnLine();
			AcGeVector3d d = r.direction();
			return std::format("AcGeRay3d(origin=({:.3f},{:.3f},{:.3f}), dir=({:.3f},{:.3f},{:.3f}))",
				p.x, p.y, p.z, d.x, d.y, d.z);
			});
}
