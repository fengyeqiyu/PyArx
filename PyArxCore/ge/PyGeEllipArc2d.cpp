#include "stdafx.h"
#include "PyGeEllipArc2d.h"
#include <format>
#include <string>

void bind_AcGeEllipArc2d(pybind11::module& m) {
	py::class_<AcGeEllipArc2d, AcGeCurve2d>(m, "AcGeEllipArc2d")
		.def(py::init<>())
		.def(py::init<const AcGePoint2d&, const AcGeVector2d&, const AcGeVector2d&, double, double>(),
			py::arg("center"), py::arg("majorAxis"), py::arg("minorAxis"),
			py::arg("majorRadius"), py::arg("minorRadius"))
		.def("center", &AcGeEllipArc2d::center)
		.def("majorRadius", &AcGeEllipArc2d::majorRadius)
		.def("minorRadius", &AcGeEllipArc2d::minorRadius)
		.def("startPoint", &AcGeEllipArc2d::startPoint)
		.def("endPoint", &AcGeEllipArc2d::endPoint)
		.def("majorAxis", &AcGeEllipArc2d::majorAxis)
		.def("minorAxis", &AcGeEllipArc2d::minorAxis)
		.def("startAng", &AcGeEllipArc2d::startAng)
		.def("endAng", &AcGeEllipArc2d::endAng)
		.def("isClockWise", &AcGeEllipArc2d::isClockWise)
		.def("isCircular", &AcGeEllipArc2d::isCircular, py::arg("tol") = AcGeContext::gTol)
		.def("isInside", &AcGeEllipArc2d::isInside, py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("setCenter", &AcGeEllipArc2d::setCenter, py::arg("center"))
		.def("setMajorRadius", &AcGeEllipArc2d::setMajorRadius, py::arg("radius"))
		.def("setMinorRadius", &AcGeEllipArc2d::setMinorRadius, py::arg("radius"))
		.def("setAxes", &AcGeEllipArc2d::setAxes, py::arg("majorAxis"), py::arg("minorAxis"))
		.def("setAngles", &AcGeEllipArc2d::setAngles, py::arg("startAngle"), py::arg("endAngle"))
		.def("intersectWithLine", [](const AcGeEllipArc2d& self, const AcGeLinearEnt2d& line, const AcGeTol& tol) {
				int intn = 0;
				AcGePoint2d p1, p2;
				bool ok = self.intersectWith(line, intn, p1, p2, tol) != Adesk::kFalse;
				return py::make_tuple(ok, intn, p1, p2);
				}, py::arg("line"), py::arg("tol") = AcGeContext::gTol)
			.def("__repr__", [](const AcGeEllipArc2d& e) {
			AcGePoint2d c = e.center();
			return std::format("AcGeEllipArc2d(center=({:.3f},{:.3f}), majR={:.3f}, minR={:.3f})",
				c.x, c.y, e.majorRadius(), e.minorRadius());
			});
}
