#include "stdafx.h"
#include "PyGeNurbCurve3d.h"
#include <format>
#include <string>

void bind_AcGeNurbCurve3d(pybind11::module& m) {
	py::class_<AcGeNurbCurve3d, AcGeSplineEnt3d>(m, "AcGeNurbCurve3d")
		.def(py::init<>())
		// Array-based ctor bridged from std::vector<AcGePoint3d> + std::vector<double> knots.
		.def(py::init([](int degree, std::vector<double> knots,
				std::vector<AcGePoint3d> ctrlPts, bool periodic) {
					AcGeKnotVector kv;
					for (double k : knots) kv.append(k);
					AcGePoint3dArray pa;
					for (const auto& p : ctrlPts) pa.append(p);
					return new AcGeNurbCurve3d(degree, kv, pa,
						periodic ? Adesk::kTrue : Adesk::kFalse);
				}),
			py::arg("degree"), py::arg("knots"), py::arg("controlPoints"),
			py::arg("periodic") = false)
		.def("numFitPoints", &AcGeNurbCurve3d::numFitPoints)
		// numWeights/weightAt: 防御式绑定 — 非有理曲线或 index 越界返回默认值，
		// 不崩不抛（AcGeNurbCurve3d 对非有理曲线访问权重数组会越界损坏堆）。
		.def("numWeights", [](const AcGeNurbCurve3d& self) {
			try {
				if (!self.isRational()) return 0;
				return self.numWeights();
			} catch (...) { return 0; }
		})
		.def("weightAt", [](const AcGeNurbCurve3d& self, int index) {
			try {
				if (index < 0) return 1.0;
				if (!self.isRational() || self.numWeights() <= 0) return 1.0;
				if (index >= self.numWeights()) return 1.0;
				return self.weightAt(index);
			} catch (...) { return 1.0; }
		}, py::arg("index"))
		.def("getFitPointAt", [](const AcGeNurbCurve3d& self, int idx) { AcGePoint3d p; Adesk::Boolean ok = self.getFitPointAt(idx, p); return py::make_tuple(ok != Adesk::kFalse, p); }, py::arg("index"))
		.def("evalMode", &AcGeNurbCurve3d::evalMode)
		.def("setFitPointAt", &AcGeNurbCurve3d::setFitPointAt, py::arg("index"), py::arg("pnt"))
		.def("addFitPointAt", &AcGeNurbCurve3d::addFitPointAt, py::arg("index"), py::arg("pnt"))
		.def("deleteFitPointAt", &AcGeNurbCurve3d::deleteFitPointAt, py::arg("index"))
		.def("purgeFitData", &AcGeNurbCurve3d::purgeFitData)
		.def("buildFitData", py::overload_cast<>(&AcGeNurbCurve3d::buildFitData))
		.def("makeRational", &AcGeNurbCurve3d::makeRational, py::arg("weight") = 1.0)
		.def("makeClosed", &AcGeNurbCurve3d::makeClosed)
		.def("makeOpen", &AcGeNurbCurve3d::makeOpen)
		.def("makePeriodic", &AcGeNurbCurve3d::makePeriodic)
		.def("makeNonPeriodic", &AcGeNurbCurve3d::makeNonPeriodic)
		.def("elevateDegree", &AcGeNurbCurve3d::elevateDegree, py::arg("plusDegree"))
		.def("addKnot", &AcGeNurbCurve3d::addKnot, py::arg("knot"))
		.def("insertKnot", &AcGeNurbCurve3d::insertKnot, py::arg("knot"))
		.def("setWeightAt", &AcGeNurbCurve3d::setWeightAt, py::arg("index"), py::arg("value"))
		.def("setEvalMode", &AcGeNurbCurve3d::setEvalMode, py::arg("evalMode") = Adesk::kFalse)
			.def("addControlPointAt", [](AcGeNurbCurve3d& self, double newKnot, const AcGePoint3d& point, double weight) {
				return self.addControlPointAt(newKnot, point, weight) != Adesk::kFalse;
				}, py::arg("newKnot"), py::arg("point"), py::arg("weight") = 1.0)
			.def("deleteControlPointAt", [](AcGeNurbCurve3d& self, int index) {
				return self.deleteControlPointAt(index) != Adesk::kFalse;
				}, py::arg("index"))
			.def("getFitTolerance", [](const AcGeNurbCurve3d& self) -> py::tuple {
				AcGeTol tol;
				bool ok = self.getFitTolerance(tol) != Adesk::kFalse;
				return py::make_tuple(ok, tol);
				})
			.def("getFitData", [](const AcGeNurbCurve3d& self) -> py::tuple {
				AcGePoint3dArray pts; AcGeTol tol; Adesk::Boolean tangentsExist;
				AcGeVector3d st, et; AcGe::KnotParameterization kp;
				Adesk::Boolean ok = self.getFitData(pts, tol, tangentsExist, st, et, kp);
				py::list plist;
				for (int i = 0; i < (int)pts.length(); ++i) plist.append(pts[i]);
				return py::make_tuple(ok != Adesk::kFalse, plist, tol, tangentsExist != Adesk::kFalse, st, et, (int)kp);
				})
			.def("setFitTolerance", [](AcGeNurbCurve3d& self, const AcGeTol& tol) {
				return self.setFitTolerance(tol) != Adesk::kFalse;
				}, py::arg("tol") = AcGeContext::gTol)
			.def("setFitDataSimple", [](AcGeNurbCurve3d& self, int degree, std::vector<AcGePoint3d> fitPoints) {
				AcGePoint3dArray pa;
				for (const auto& p : fitPoints) pa.append(p);
				self.setFitData(degree, pa, AcGeContext::gTol);
				return self;
				}, py::arg("degree"), py::arg("fitPoints"), py::return_value_policy::reference)
			.def("hardTrimByParams", [](AcGeNurbCurve3d& self, double newStart, double newEnd) {
				self.hardTrimByParams(newStart, newEnd);
				return self;
				}, py::arg("newStartParam"), py::arg("newEndParam"), py::return_value_policy::reference)
			.def("joinWith", [](AcGeNurbCurve3d& self, const AcGeNurbCurve3d& curve) {
				self.joinWith(curve);
				return self;
				}, py::arg("curve"), py::return_value_policy::reference)
		.def("__repr__", [](const AcGeNurbCurve3d& s) {
			return std::format("AcGeNurbCurve3d(degree={}, ctrlPts={})",
				s.degree(), s.numControlPoints());
			});
}
