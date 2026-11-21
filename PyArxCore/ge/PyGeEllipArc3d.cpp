#include "stdafx.h"
#include "PyGeEllipArc3d.h"
#include <format>
#include <string>

void bind_AcGeEllipArc3d(pybind11::module& m) {
	py::class_<AcGeEllipArc3d, AcGeCurve3d>(m, "AcGeEllipArc3d")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&, const AcGeVector3d&, const AcGeVector3d&, double, double>(),
			py::arg("center"), py::arg("majorAxis"), py::arg("minorAxis"),
			py::arg("majorRadius"), py::arg("minorRadius"))
		.def(py::init<const AcGePoint3d&, const AcGeVector3d&, const AcGeVector3d&, double, double, double, double>(),
			py::arg("center"), py::arg("majorAxis"), py::arg("minorAxis"),
			py::arg("majorRadius"), py::arg("minorRadius"),
			py::arg("ang1"), py::arg("ang2"))
		.def("center", &AcGeEllipArc3d::center)
		.def("majorRadius", &AcGeEllipArc3d::majorRadius)
		.def("minorRadius", &AcGeEllipArc3d::minorRadius)
		.def("majorAxis", &AcGeEllipArc3d::majorAxis)
		.def("minorAxis", &AcGeEllipArc3d::minorAxis)
		.def("normal", &AcGeEllipArc3d::normal)
		.def("startPoint", &AcGeEllipArc3d::startPoint)
		.def("endPoint", &AcGeEllipArc3d::endPoint)
		.def("startAng", &AcGeEllipArc3d::startAng)
		.def("endAng", &AcGeEllipArc3d::endAng)
		.def("isCircular", &AcGeEllipArc3d::isCircular, py::arg("tol") = AcGeContext::gTol)
		.def("isInside", &AcGeEllipArc3d::isInside, py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("setCenter", &AcGeEllipArc3d::setCenter, py::arg("center"))
		.def("setMajorRadius", &AcGeEllipArc3d::setMajorRadius, py::arg("radius"))
		.def("setMinorRadius", &AcGeEllipArc3d::setMinorRadius, py::arg("radius"))
		.def("setAxes", &AcGeEllipArc3d::setAxes, py::arg("majorAxis"), py::arg("minorAxis"))
		.def("setAngles", &AcGeEllipArc3d::setAngles, py::arg("startAngle"), py::arg("endAngle"))
			.def("closestPointToPlane", [](const AcGeEllipArc3d& self, const AcGePlanarEnt& plane, const AcGeTol& tol) {
				AcGePoint3d onPlane;
				AcGePoint3d p = self.closestPointToPlane(plane, onPlane, tol);
				return py::make_tuple(p, onPlane);
				}, py::arg("plane"), py::arg("tol") = AcGeContext::gTol)
			.def("getPlane", [](const AcGeEllipArc3d& self) {
				AcGePlane plane;
				self.getPlane(plane);
				return plane;
				})
		.def("intersectWithLine", [](const AcGeEllipArc3d& self, const AcGeLinearEnt3d& line, const AcGeTol& tol) {
					int intn = 0;
					AcGePoint3d p1, p2;
					bool ok = self.intersectWith(line, intn, p1, p2, tol) != Adesk::kFalse;
					return py::make_tuple(ok, intn, p1, p2);
					}, py::arg("line"), py::arg("tol") = AcGeContext::gTol)
				.def("intersectWithPlane", [](const AcGeEllipArc3d& self, const AcGePlanarEnt& plane, const AcGeTol& tol) {
					int intn = 0;
					AcGePoint3d p1, p2;
					bool ok = self.intersectWith(plane, intn, p1, p2, tol) != Adesk::kFalse;
					return py::make_tuple(ok, intn, p1, p2);
					}, py::arg("plane"), py::arg("tol") = AcGeContext::gTol)
			.def("__repr__", [](const AcGeEllipArc3d& e) {
			AcGePoint3d c = e.center();
			return std::format("AcGeEllipArc3d(center=({:.3f},{:.3f},{:.3f}), majR={:.3f}, minR={:.3f})",
				c.x, c.y, c.z, e.majorRadius(), e.minorRadius());
			});
}
