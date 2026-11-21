#include "stdafx.h"
#include "PyGeCurveCurveInt3d.h"
#include <format>
#include <string>

void bind_AcGeCurveCurveInt3d(pybind11::module& m) {
	py::class_<AcGeCurveCurveInt3d, AcGeEntity3d>(m, "AcGeCurveCurveInt3d")
		.def(py::init<>())
		.def(py::init<const AcGeCurve3d&, const AcGeCurve3d&, const AcGeVector3d&, const AcGeTol&>(),
			py::arg("curve1"), py::arg("curve2"),
			py::arg("planeNormal") = AcGeVector3d::kIdentity,
			py::arg("tol") = AcGeContext::gTol)
		.def("numIntPoints", &AcGeCurveCurveInt3d::numIntPoints)
		.def("intPoint", &AcGeCurveCurveInt3d::intPoint, py::arg("intNum"))
		.def("isTangential", &AcGeCurveCurveInt3d::isTangential, py::arg("intNum"))
		.def("isTransversal", &AcGeCurveCurveInt3d::isTransversal, py::arg("intNum"))
		.def("intPointTol", &AcGeCurveCurveInt3d::intPointTol, py::arg("intNum"))
		.def("tolerance", &AcGeCurveCurveInt3d::tolerance)
		.def("planeNormal", &AcGeCurveCurveInt3d::planeNormal)
		.def("overlapCount", &AcGeCurveCurveInt3d::overlapCount)
		.def("overlapDirection", &AcGeCurveCurveInt3d::overlapDirection)
		.def("changeCurveOrder", &AcGeCurveCurveInt3d::changeCurveOrder)
		.def("orderWrt1", &AcGeCurveCurveInt3d::orderWrt1)
		.def("orderWrt2", &AcGeCurveCurveInt3d::orderWrt2)
		.def("getIntParams", [](const AcGeCurveCurveInt3d& self, int intNum) {
				double p1 = 0.0, p2 = 0.0;
				self.getIntParams(intNum, p1, p2);
				return py::make_tuple(p1, p2);
				}, py::arg("intNum"), "Return (paramOnCurve1, paramOnCurve2) at intersection")
			.def("getIntRanges", [](const AcGeCurveCurveInt3d& self) {
				AcGeInterval r1, r2;
				self.getIntRanges(r1, r2);
				return py::make_tuple(r1.lowerBound(), r1.upperBound(), r2.lowerBound(), r2.upperBound());
				}, "Return (range1_lo, range1_hi, range2_lo, range2_hi)")
			.def("getPointOnCurve1", [](const AcGeCurveCurveInt3d& self, int intNum) {
				AcGePointOnCurve3d p;
				self.getPointOnCurve1(intNum, p);
				return p;
				}, py::arg("intNum"), "Return AcGePointOnCurve3d at intersection on curve1")
			.def("getPointOnCurve2", [](const AcGeCurveCurveInt3d& self, int intNum) {
				AcGePointOnCurve3d p;
				self.getPointOnCurve2(intNum, p);
				return p;
				}, py::arg("intNum"), "Return AcGePointOnCurve3d at intersection on curve2")
			.def("getIntConfigs", [](const AcGeCurveCurveInt3d& self, int intNum) {
				AcGe::AcGeXConfig c1, c2;
				self.getIntConfigs(intNum, c1, c2);
				return py::make_tuple((int)c1, (int)c2);
				}, py::arg("intNum"), "Return (config1wrt2, config2wrt1) as AcGeXConfig ints")
			.def("__repr__", [](const AcGeCurveCurveInt3d& c) {
			return std::format("AcGeCurveCurveInt3d(nInt={})", c.numIntPoints());
			});
}
