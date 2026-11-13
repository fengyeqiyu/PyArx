#include "stdafx.h"
#include "PyGeCircArc2d.h"
#include <format>
#include <string>

void bind_AcGeCircArc2d(pybind11::module& m) {
	py::class_<AcGeCircArc2d, AcGeCurve2d>(m, "AcGeCircArc2d")
		.def(py::init<>())
		.def(py::init<const AcGeCircArc2d&>(), py::arg("src"))
		.def(py::init<const AcGePoint2d&, double>(),
			py::arg("center"), py::arg("radius"))
		.def(py::init<const AcGePoint2d&, double, double, double, const AcGeVector2d&, Adesk::Boolean>(),
			py::arg("center"), py::arg("radius"),
			py::arg("startAngle"), py::arg("endAngle"),
			py::arg("refVec") = AcGeVector2d::kXAxis,
			py::arg("isClockWise") = Adesk::kFalse)
		.def(py::init<const AcGePoint2d&, const AcGePoint2d&, const AcGePoint2d&>(),
			py::arg("startPoint"), py::arg("point"), py::arg("endPoint"))
		.def("center", &AcGeCircArc2d::center)
		.def("radius", &AcGeCircArc2d::radius)
		.def("startAng", &AcGeCircArc2d::startAng)
		.def("endAng", &AcGeCircArc2d::endAng)
		.def("startPoint", &AcGeCircArc2d::startPoint)
		.def("endPoint", &AcGeCircArc2d::endPoint)
		.def("isInside", &AcGeCircArc2d::isInside,
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("isClockWise", &AcGeCircArc2d::isClockWise)
		.def("refVec", &AcGeCircArc2d::refVec)
		.def("setCenter", &AcGeCircArc2d::setCenter, py::arg("center"))
		.def("setRadius", &AcGeCircArc2d::setRadius, py::arg("radius"))
		.def("setAngles", &AcGeCircArc2d::setAngles, py::arg("startAng"), py::arg("endAng"))
		.def("setRefVec", &AcGeCircArc2d::setRefVec, py::arg("vec"))
		.def("intersectWithLine", [](const AcGeCircArc2d& self, const AcGeLinearEnt2d& line, const AcGeTol& tol) {
				int intn = 0;
				AcGePoint2d p1, p2;
				bool ok = self.intersectWith(line, intn, p1, p2, tol) != Adesk::kFalse;
				return py::make_tuple(ok, intn, p1, p2);
				}, py::arg("line"), py::arg("tol") = AcGeContext::gTol)
			.def("intersectWithArc", [](const AcGeCircArc2d& self, const AcGeCircArc2d& arc, const AcGeTol& tol) {
				int intn = 0;
				AcGePoint2d p1, p2;
				bool ok = self.intersectWith(arc, intn, p1, p2, tol) != Adesk::kFalse;
				return py::make_tuple(ok, intn, p1, p2);
				}, py::arg("arc"), py::arg("tol") = AcGeContext::gTol)
			.def("__repr__", [](const AcGeCircArc2d& a) {
			AcGePoint2d c = a.center();
			return std::format("AcGeCircArc2d(center=({:.3f},{:.3f}), r={:.3f}, [{:.3f},{:.3f}])",
				c.x, c.y, a.radius(), a.startAng(), a.endAng());
			});
}
