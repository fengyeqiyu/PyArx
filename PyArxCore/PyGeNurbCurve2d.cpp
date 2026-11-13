#include "stdafx.h"
#include "PyGeNurbCurve2d.h"
#include <format>
#include <string>

void bind_AcGeNurbCurve2d(pybind11::module& m) {
	py::class_<AcGeNurbCurve2d, AcGeSplineEnt2d>(m, "AcGeNurbCurve2d")
		.def(py::init<>())
		// Array-based ctor bridged from std::vector<AcGePoint2d> + std::vector<double> knots.
		.def(py::init([](int degree, std::vector<double> knots,
				std::vector<AcGePoint2d> ctrlPts, bool periodic) {
					AcGeKnotVector kv;
					for (double k : knots) kv.append(k);
					AcGePoint2dArray pa;
					for (const auto& p : ctrlPts) pa.append(p);
					return new AcGeNurbCurve2d(degree, kv, pa,
						periodic ? Adesk::kTrue : Adesk::kFalse);
				}),
			py::arg("degree"), py::arg("knots"), py::arg("controlPoints"),
			py::arg("periodic") = false)
		.def("numFitPoints", &AcGeNurbCurve2d::numFitPoints)
		.def("numWeights", &AcGeNurbCurve2d::numWeights)
		.def("weightAt", &AcGeNurbCurve2d::weightAt, py::arg("index"))
		.def("getFitPointAt", [](const AcGeNurbCurve2d& self, int idx) { AcGePoint2d p; Adesk::Boolean ok = self.getFitPointAt(idx, p); return py::make_tuple(ok != Adesk::kFalse, p); }, py::arg("index"))
		.def("evalMode", &AcGeNurbCurve2d::evalMode)
		.def("setFitPointAt", &AcGeNurbCurve2d::setFitPointAt, py::arg("index"), py::arg("pnt"))
		.def("addFitPointAt", &AcGeNurbCurve2d::addFitPointAt, py::arg("index"), py::arg("pnt"))
		.def("deleteFitPointAt", &AcGeNurbCurve2d::deleteFitPointAt, py::arg("index"))
		.def("purgeFitData", &AcGeNurbCurve2d::purgeFitData)
		.def("buildFitData", py::overload_cast<>(&AcGeNurbCurve2d::buildFitData))
		.def("makeRational", &AcGeNurbCurve2d::makeRational, py::arg("weight") = 1.0)
		.def("makeClosed", &AcGeNurbCurve2d::makeClosed)
		.def("makeOpen", &AcGeNurbCurve2d::makeOpen)
		.def("makePeriodic", &AcGeNurbCurve2d::makePeriodic)
		.def("makeNonPeriodic", &AcGeNurbCurve2d::makeNonPeriodic)
		.def("elevateDegree", &AcGeNurbCurve2d::elevateDegree, py::arg("plusDegree"))
		.def("addKnot", &AcGeNurbCurve2d::addKnot, py::arg("knot"))
		.def("insertKnot", &AcGeNurbCurve2d::insertKnot, py::arg("knot"))
		.def("setWeightAt", &AcGeNurbCurve2d::setWeightAt, py::arg("index"), py::arg("value"))
		.def("setEvalMode", &AcGeNurbCurve2d::setEvalMode, py::arg("evalMode") = Adesk::kFalse)
		.def("addControlPointAt", [](AcGeNurbCurve2d& self, double newKnot, const AcGePoint2d& point, double weight) {
				return self.addControlPointAt(newKnot, point, weight) != Adesk::kFalse;
				}, py::arg("newKnot"), py::arg("point"), py::arg("weight") = 1.0)
			.def("deleteControlPointAt", [](AcGeNurbCurve2d& self, int index) {
				return self.deleteControlPointAt(index) != Adesk::kFalse;
				}, py::arg("index"))
			.def("getFitTolerance", [](const AcGeNurbCurve2d& self) -> py::tuple {
				AcGeTol tol;
				bool ok = self.getFitTolerance(tol) != Adesk::kFalse;
				return py::make_tuple(ok, tol);
				})
			.def("getFitData", [](const AcGeNurbCurve2d& self) -> py::tuple {
				AcGePoint2dArray pts; AcGeTol tol; Adesk::Boolean tangentsExist;
				AcGeVector2d st, et; AcGe::KnotParameterization kp;
				Adesk::Boolean ok = self.getFitData(pts, tol, tangentsExist, st, et, kp);
				py::list plist;
				for (int i = 0; i < (int)pts.length(); ++i) plist.append(pts[i]);
				return py::make_tuple(ok != Adesk::kFalse, plist, tol, tangentsExist != Adesk::kFalse, st, et, (int)kp);
				})
			.def("setFitTolerance", [](AcGeNurbCurve2d& self, const AcGeTol& tol) {
				return self.setFitTolerance(tol) != Adesk::kFalse;
				}, py::arg("tol") = AcGeContext::gTol)
			.def("setFitDataSimple", [](AcGeNurbCurve2d& self, int degree, std::vector<AcGePoint2d> fitPoints) {
				AcGePoint2dArray pa;
				for (const auto& p : fitPoints) pa.append(p);
				self.setFitData(degree, pa, AcGeContext::gTol);
				return self;
				}, py::arg("degree"), py::arg("fitPoints"), py::return_value_policy::reference)
			.def("hardTrimByParams", [](AcGeNurbCurve2d& self, double newStart, double newEnd) {
				self.hardTrimByParams(newStart, newEnd);
				return self;
				}, py::arg("newStartParam"), py::arg("newEndParam"), py::return_value_policy::reference)
			.def("joinWith", [](AcGeNurbCurve2d& self, const AcGeNurbCurve2d& curve) {
				self.joinWith(curve);
				return self;
				}, py::arg("curve"), py::return_value_policy::reference)
			.def("__repr__", [](const AcGeNurbCurve2d& s) {
			return std::format("AcGeNurbCurve2d(degree={}, ctrlPts={})",
				s.degree(), s.numControlPoints());
			});
}
