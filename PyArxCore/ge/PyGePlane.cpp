#include "stdafx.h"
#include "PyGePlane.h"
#include <format>
#include <string>

void bind_AcGePlane(pybind11::module& m) {
	py::class_<AcGePlane, AcGePlanarEnt>(m, "AcGePlane")
		.def(py::init<>())
		.def(py::init<const AcGePlane&>(), py::arg("src"))
		.def(py::init<const AcGePoint3d&, const AcGeVector3d&>(),
			py::arg("origin"), py::arg("normal"))
		.def(py::init<double, double, double, double>(),
			py::arg("a"), py::arg("b"), py::arg("c"), py::arg("d"))
		.def_readonly_static("kXYPlane", &AcGePlane::kXYPlane)
		.def_readonly_static("kYZPlane", &AcGePlane::kYZPlane)
		.def_readonly_static("kZXPlane", &AcGePlane::kZXPlane)
		.def("signedDistanceTo", &AcGePlane::signedDistanceTo, py::arg("pnt"))
		.def("set", py::overload_cast<const AcGePoint3d&, const AcGeVector3d&>(&AcGePlane::set), py::arg("origin"), py::arg("normal"))
		.def("set", py::overload_cast<double, double, double, double>(&AcGePlane::set), py::arg("a"), py::arg("b"), py::arg("c"), py::arg("d"))
		.def("intersectWithLine", [](const AcGePlane& self, const AcGeLinearEnt3d& line, const AcGeTol& tol) { AcGePoint3d p; Adesk::Boolean ok = self.intersectWith(line, p, tol); return py::make_tuple(ok != Adesk::kFalse, p); }, py::arg("line"), py::arg("tol") = AcGeContext::gTol)
			.def("intersectWith", [](const AcGePlane& self, const AcGePlane& other, const AcGeTol& tol) -> py::tuple {
				AcGeLine3d line;
				bool ok = self.intersectWith(other, line, tol) != Adesk::kFalse;
				return py::make_tuple(ok, line);
				}, py::arg("other"), py::arg("tol") = AcGeContext::gTol)
		.def("__repr__", [](const AcGePlane& pl) {
			AcGePoint3d o = pl.pointOnPlane();
			AcGeVector3d n = pl.normal();
			return std::format("AcGePlane(origin=({:.3f},{:.3f},{:.3f}), normal=({:.3f},{:.3f},{:.3f}))",
				o.x, o.y, o.z, n.x, n.y, n.z);
			});
}
