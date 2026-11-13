#include "stdafx.h"
#include "PyGeCircArc3d.h"
#include <format>
#include <string>

void bind_AcGeCircArc3d(pybind11::module& m) {
	py::class_<AcGeCircArc3d, AcGeCurve3d>(m, "AcGeCircArc3d")
		.def(py::init<>())
		.def(py::init<const AcGeCircArc3d&>(), py::arg("src"))
		.def(py::init<const AcGePoint3d&, const AcGeVector3d&, double>(),
			py::arg("center"), py::arg("normal"), py::arg("radius"))
		.def(py::init<const AcGePoint3d&, const AcGeVector3d&, const AcGeVector3d&, double, double, double>(),
			py::arg("center"), py::arg("normal"), py::arg("refVec"),
			py::arg("radius"), py::arg("startAngle"), py::arg("endAngle"))
		.def(py::init<const AcGePoint3d&, const AcGePoint3d&, const AcGePoint3d&>(),
			py::arg("startPoint"), py::arg("point"), py::arg("endPoint"))
		.def("center", &AcGeCircArc3d::center)
		.def("normal", &AcGeCircArc3d::normal)
		.def("radius", &AcGeCircArc3d::radius)
		.def("startAng", &AcGeCircArc3d::startAng)
		.def("endAng", &AcGeCircArc3d::endAng)
		.def("startPoint", &AcGeCircArc3d::startPoint)
		.def("endPoint", &AcGeCircArc3d::endPoint)
		.def("isInside", &AcGeCircArc3d::isInside,
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("refVec", &AcGeCircArc3d::refVec)
		.def("setCenter", &AcGeCircArc3d::setCenter, py::arg("center"))
		.def("setRadius", &AcGeCircArc3d::setRadius, py::arg("radius"))
		.def("setAngles", &AcGeCircArc3d::setAngles, py::arg("startAng"), py::arg("endAng"))
		.def("setAxes", &AcGeCircArc3d::setAxes, py::arg("normal"), py::arg("refVec"))
		.def("closestPointToPlane", [](const AcGeCircArc3d& self, const AcGePlanarEnt& plane, const AcGeTol& tol) {
			AcGePoint3d onPlane;
			AcGePoint3d p = self.closestPointToPlane(plane, onPlane, tol);
			return py::make_tuple(p, onPlane);
		}, py::arg("plane"), py::arg("tol") = AcGeContext::gTol)
		.def("getPlane", [](const AcGeCircArc3d& self) {
			AcGePlane plane;
			self.getPlane(plane);
			return plane;
		})
		.def("intersectWithLine", [](const AcGeCircArc3d& self, const AcGeLinearEnt3d& line, const AcGeTol& tol) {
			int intn = 0;
			AcGePoint3d p1, p2;
			bool ok = self.intersectWith(line, intn, p1, p2, tol) != Adesk::kFalse;
			return py::make_tuple(ok, intn, p1, p2);
		}, py::arg("line"), py::arg("tol") = AcGeContext::gTol)
		.def("intersectWithArc", [](const AcGeCircArc3d& self, const AcGeCircArc3d& arc, const AcGeTol& tol) {
			int intn = 0;
			AcGePoint3d p1, p2;
			bool ok = self.intersectWith(arc, intn, p1, p2, tol) != Adesk::kFalse;
			return py::make_tuple(ok, intn, p1, p2);
		}, py::arg("arc"), py::arg("tol") = AcGeContext::gTol)
		.def("intersectWithPlane", [](const AcGeCircArc3d& self, const AcGePlanarEnt& plane, const AcGeTol& tol) {
			int intn = 0;
			AcGePoint3d p1, p2;
			bool ok = self.intersectWith(plane, intn, p1, p2, tol) != Adesk::kFalse;
			return py::make_tuple(ok, intn, p1, p2);
		}, py::arg("plane"), py::arg("tol") = AcGeContext::gTol)
		.def("__repr__", [](const AcGeCircArc3d& a) {
			AcGePoint3d c = a.center();
			return std::format("AcGeCircArc3d(center=({:.3f},{:.3f},{:.3f}), r={:.3f}, [{:.3f},{:.3f}])",
				c.x, c.y, c.z, a.radius(), a.startAng(), a.endAng());
			});
}
