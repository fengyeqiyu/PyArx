#include "stdafx.h"
#include "PyGePolyline3d.h"
#include <format>
#include <string>

void bind_AcGePolyline3d(pybind11::module& m) {
	py::class_<AcGePolyline3d, AcGeSplineEnt3d>(m, "AcGePolyline3d")
		// ---- 构造（geplin3d.h 全部 GE_DLLEXPIMPORT 构造器）----
		.def(py::init<>())
		.def(py::init<const AcGePolyline3d&>(), py::arg("src"))
		.def(py::init([](std::vector<AcGePoint3d> pts) {
			AcGePoint3dArray pa;
			for (const auto& p : pts) pa.append(p);
			return new AcGePolyline3d(pa);
		}), py::arg("points"))
		// AcGePolyline3d(const AcGeKnotVector& knots, const AcGePoint3dArray& cntrlPnts)
		.def(py::init([](std::vector<double> knots, std::vector<AcGePoint3d> pts) {
			AcGeKnotVector kv;
			for (double k : knots) kv.append(k);
			AcGePoint3dArray pa;
			for (const auto& p : pts) pa.append(p);
			return new AcGePolyline3d(kv, pa);
		}), py::arg("knots"), py::arg("points"))
		// AcGePolyline3d(const AcGeCurve3d& crv, double apprEps) —— 用折线逼近给定曲线
		.def(py::init([](const AcGeCurve3d& crv, double apprEps) {
			return new AcGePolyline3d(crv, apprEps);
		}), py::arg("crv"), py::arg("apprEps"))

		// ---- 插值数据 ----
		.def("numFitPoints", &AcGePolyline3d::numFitPoints)
		// fitPointAt: 越界/异常返回 kOrigin，不崩不抛
		.def("fitPointAt", [](const AcGePolyline3d& self, int idx) -> AcGePoint3d {
			try {
				if (idx < 0 || idx >= self.numFitPoints()) {
					acutPrintf(L"[PyArx] AcGePolyline3d.fitPointAt 索引越界 (%d/%d)\n", idx, self.numFitPoints());
					return AcGePoint3d::kOrigin;
				}
				return self.fitPointAt(idx);
			} catch (...) {
				acutPrintf(L"[PyArx] AcGePolyline3d.fitPointAt 异常 idx=%d\n", idx);
				return AcGePoint3d::kOrigin;
			}
		}, py::arg("index"))
		// setFitPointAt: setter 返回 bool（成功 true / 失败 false），异常不崩不抛
		.def("setFitPointAt", [](AcGePolyline3d& self, int idx, const AcGePoint3d& point) -> bool {
			try {
				if (idx < 0 || idx >= self.numFitPoints()) {
					acutPrintf(L"[PyArx] AcGePolyline3d.setFitPointAt 索引越界 (%d/%d)\n", idx, self.numFitPoints());
					return false;
				}
				self.setFitPointAt(idx, point);
				return true;
			} catch (...) {
				acutPrintf(L"[PyArx] AcGePolyline3d.setFitPointAt 异常 idx=%d\n", idx);
				return false;
			}
		}, py::arg("index"), py::arg("point"))
		.def("__repr__", [](const AcGePolyline3d& p) {
			return std::format("AcGePolyline3d(fitPts={})", p.numFitPoints());
		});
}
