#include "stdafx.h"
#include "PyGeCurveCurveInt2d.h"
#include <format>
#include <string>

void bind_AcGeCurveCurveInt2d(pybind11::module& m) {
	py::class_<AcGeCurveCurveInt2d, AcGeEntity2d>(m, "AcGeCurveCurveInt2d")
		.def(py::init<>())
		.def(py::init<const AcGeCurve2d&, const AcGeCurve2d&, const AcGeTol&>(),
			py::arg("curve1"), py::arg("curve2"), py::arg("tol") = AcGeContext::gTol)
		.def("numIntPoints", &AcGeCurveCurveInt2d::numIntPoints)
		.def("intPoint", &AcGeCurveCurveInt2d::intPoint, py::arg("intNum"))
		.def("isTangential", &AcGeCurveCurveInt2d::isTangential, py::arg("intNum"))
		.def("isTransversal", &AcGeCurveCurveInt2d::isTransversal, py::arg("intNum"))
		.def("intPointTol", &AcGeCurveCurveInt2d::intPointTol, py::arg("intNum"))
		.def("tolerance", &AcGeCurveCurveInt2d::tolerance)
		.def("overlapCount", &AcGeCurveCurveInt2d::overlapCount)
		.def("overlapDirection", &AcGeCurveCurveInt2d::overlapDirection)
		.def("changeCurveOrder", &AcGeCurveCurveInt2d::changeCurveOrder)
		.def("orderWrt1", &AcGeCurveCurveInt2d::orderWrt1)
		.def("orderWrt2", &AcGeCurveCurveInt2d::orderWrt2)
		.def("getIntParams", [](const AcGeCurveCurveInt2d& self, int intNum) {
				double p1 = 0.0, p2 = 0.0;
				self.getIntParams(intNum, p1, p2);
				return py::make_tuple(p1, p2);
				}, py::arg("intNum"), "Return (paramOnCurve1, paramOnCurve2) at intersection")
			.def("getIntRanges", [](const AcGeCurveCurveInt2d& self) {
				AcGeInterval r1, r2;
				self.getIntRanges(r1, r2);
				return py::make_tuple(r1.lowerBound(), r1.upperBound(), r2.lowerBound(), r2.upperBound());
				}, "Return (range1_lo, range1_hi, range2_lo, range2_hi)")
			.def("getPointOnCurve1", [](const AcGeCurveCurveInt2d& self, int intNum) {
				AcGePointOnCurve2d p;
				self.getPointOnCurve1(intNum, p);
				return p;
				}, py::arg("intNum"), "Return AcGePointOnCurve2d at intersection on curve1")
			.def("getPointOnCurve2", [](const AcGeCurveCurveInt2d& self, int intNum) {
				AcGePointOnCurve2d p;
				self.getPointOnCurve2(intNum, p);
				return p;
				}, py::arg("intNum"), "Return AcGePointOnCurve2d at intersection on curve2")
			.def("getIntConfigs", [](const AcGeCurveCurveInt2d& self, int intNum) {
				AcGe::AcGeXConfig c1, c2;
				self.getIntConfigs(intNum, c1, c2);
				return py::make_tuple((int)c1, (int)c2);
				}, py::arg("intNum"), "Return (config1wrt2, config2wrt1) as AcGeXConfig ints")
			.def("__repr__", [](const AcGeCurveCurveInt2d& c) {
			return std::format("AcGeCurveCurveInt2d(nInt={})", c.numIntPoints());
			});
}
