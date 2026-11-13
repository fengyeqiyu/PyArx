#include "stdafx.h"
#include "PyGePolyline2d.h"
#include <format>
#include <string>

void bind_AcGePolyline2d(pybind11::module& m) {
	py::class_<AcGePolyline2d, AcGeSplineEnt2d>(m, "AcGePolyline2d")
		// ---- 构造（geplin2d.h 全部 GE_DLLEXPIMPORT 构造器）----
		.def(py::init<>())
		.def(py::init<const AcGePolyline2d&>(), py::arg("src"))
		.def(py::init([](std::vector<AcGePoint2d> pts) {
			AcGePoint2dArray pa;
			for (const auto& p : pts) pa.append(p);
			return new AcGePolyline2d(pa);
		}), py::arg("points"))
		// AcGePolyline2d(const AcGeKnotVector& knots, const AcGePoint2dArray& points)
		.def(py::init([](std::vector<double> knots, std::vector<AcGePoint2d> pts) {
			AcGeKnotVector kv;
			for (double k : knots) kv.append(k);
			AcGePoint2dArray pa;
			for (const auto& p : pts) pa.append(p);
			return new AcGePolyline2d(kv, pa);
		}), py::arg("knots"), py::arg("points"))
		// AcGePolyline2d(const AcGeCurve2d& crv, double apprEps) —— 用折线逼近给定曲线
		.def(py::init([](const AcGeCurve2d& crv, double apprEps) {
			return new AcGePolyline2d(crv, apprEps);
		}), py::arg("crv"), py::arg("apprEps"))

		// ---- 插值数据 ----
		.def("numFitPoints", &AcGePolyline2d::numFitPoints)
		// fitPointAt: 越界/异常返回 kOrigin，不崩不抛
		.def("fitPointAt", [](const AcGePolyline2d& self, int idx) -> AcGePoint2d {
			try {
				if (idx < 0 || idx >= self.numFitPoints()) {
					acutPrintf(L"[PyArx] AcGePolyline2d.fitPointAt 索引越界 (%d/%d)\n", idx, self.numFitPoints());
					return AcGePoint2d::kOrigin;
				}
				return self.fitPointAt(idx);
			} catch (...) {
				acutPrintf(L"[PyArx] AcGePolyline2d.fitPointAt 异常 idx=%d\n", idx);
				return AcGePoint2d::kOrigin;
			}
		}, py::arg("index"))
		// setFitPointAt: setter 返回 bool（成功 true / 失败 false），异常不崩不抛
		.def("setFitPointAt", [](AcGePolyline2d& self, int idx, const AcGePoint2d& point) -> bool {
			try {
				if (idx < 0 || idx >= self.numFitPoints()) {
					acutPrintf(L"[PyArx] AcGePolyline2d.setFitPointAt 索引越界 (%d/%d)\n", idx, self.numFitPoints());
					return false;
				}
				self.setFitPointAt(idx, point);
				return true;
			} catch (...) {
				acutPrintf(L"[PyArx] AcGePolyline2d.setFitPointAt 异常 idx=%d\n", idx);
				return false;
			}
		}, py::arg("index"), py::arg("point"))
		.def("__repr__", [](const AcGePolyline2d& p) {
			return std::format("AcGePolyline2d(fitPts={})", p.numFitPoints());
		});
}
