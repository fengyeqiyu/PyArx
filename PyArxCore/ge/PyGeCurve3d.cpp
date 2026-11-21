#include "stdafx.h"
#include "PyGeCurve3d.h"
#include <format>
#include <string>

void bind_AcGeCurve3d(pybind11::module& m) {
	// Abstract base: no py::init.
	py::class_<AcGeCurve3d, AcGeEntity3d>(m, "AcGeCurve3d")
		.def("distanceTo",
			py::overload_cast<const AcGePoint3d&, const AcGeTol&>(&AcGeCurve3d::distanceTo, py::const_),
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("closestPointTo",
			py::overload_cast<const AcGePoint3d&, const AcGeTol&>(&AcGeCurve3d::closestPointTo, py::const_),
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("isClosed", &AcGeCurve3d::isClosed,
			py::arg("tol") = AcGeContext::gTol)
		.def("isOn",
			py::overload_cast<const AcGePoint3d&, const AcGeTol&>(&AcGeCurve3d::isOn, py::const_),
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("paramOf", &AcGeCurve3d::paramOf,
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("getInterval", [](const AcGeCurve3d& self) {
			AcGeInterval iv;
			self.getInterval(iv);
			return py::make_tuple(iv.lowerBound(), iv.upperBound());
			}, "Return (lowerBound, upperBound) of the parameter interval")
		.def("setInterval", [](AcGeCurve3d& self, double lo, double hi) {
			AcGeInterval iv(lo, hi);
			return self.setInterval(iv) != Adesk::kFalse;
			}, py::arg("lower"), py::arg("upper"))
		.def("evalPoint", py::overload_cast<double>(&AcGeCurve3d::evalPoint, py::const_), py::arg("param"))
		.def("length", &AcGeCurve3d::length,
			py::arg("fromParam"), py::arg("toParam"),
			py::arg("tol") = AcGeContext::gTol.equalPoint())
		.def("hasStartPoint", [](const AcGeCurve3d& self) {
			AcGePoint3d p;
			bool ok = self.hasStartPoint(p);
			return py::make_tuple(ok, p);
			})
		.def("hasEndPoint", [](const AcGeCurve3d& self) {
			AcGePoint3d p;
			bool ok = self.hasEndPoint(p);
			return py::make_tuple(ok, p);
			})
		.def("reverseParam", &AcGeCurve3d::reverseParam, py::return_value_policy::reference)
		.def("resetInterval", py::overload_cast<>(&AcGeCurve3d::setInterval))
		.def("paramAtLength", &AcGeCurve3d::paramAtLength, py::arg("datumParam"), py::arg("length"), py::arg("posParamDir") = Adesk::kTrue, py::arg("tol") = AcGeContext::gTol.equalPoint())
		.def("isPeriodic", [](const AcGeCurve3d& self) { double period = 0.0; Adesk::Boolean ok = self.isPeriodic(period); return py::make_tuple(ok != Adesk::kFalse, period); })
		.def("isOnParam", py::overload_cast<double, const AcGeTol&>(&AcGeCurve3d::isOn, py::const_), py::arg("param"), py::arg("tol") = AcGeContext::gTol)
		.def("distanceToCurve", py::overload_cast<const AcGeCurve3d&, const AcGeTol&>(&AcGeCurve3d::distanceTo, py::const_), py::arg("other"), py::arg("tol") = AcGeContext::gTol)
		.def("isLinear", [](const AcGeCurve3d& c, const AcGeTol& tol) -> py::tuple {
			AcGeLine3d line;
			bool ok = c.isLinear(line, tol) != Adesk::kFalse;
			return py::make_tuple(ok, line);
			}, py::arg("tol") = AcGeContext::gTol)
		.def("isPlanar", [](const AcGeCurve3d& c, const AcGeTol& tol) -> py::tuple {
			AcGePlane plane;
			bool ok = c.isPlanar(plane, tol) != Adesk::kFalse;
			return py::make_tuple(ok, plane);
			}, py::arg("tol") = AcGeContext::gTol)
		.def("isCoplanarWith", [](const AcGeCurve3d& c, const AcGeCurve3d& other, const AcGeTol& tol) -> py::tuple {
			AcGePlane plane;
			bool ok = c.isCoplanarWith(other, plane, tol) != Adesk::kFalse;
			return py::make_tuple(ok, plane);
			}, py::arg("other"), py::arg("tol") = AcGeContext::gTol)
		.def("isDegenerate", [](const AcGeCurve3d& c, const AcGeTol& tol) -> py::tuple {
			AcGe::EntityId type = static_cast<AcGe::EntityId>(0);
			bool ok = c.isDegenerate(type, tol) != Adesk::kFalse;
			return py::make_tuple(ok, ok ? (int)type : 0);
			}, py::arg("tol") = AcGeContext::gTol)
			.def("getSamplePoints", [](const AcGeCurve3d& c, int numSample) {
				AcGePoint3dArray pts;
				c.getSamplePoints(numSample, pts);
				return pts;
				}, py::arg("numSample"))
		// 说明: getSamplePoints 返回 AcGePoint3dArray(经 AcArray type_caster 转 list)。
		// debug 构建(msvcrtd)下, AcGe 在 acad(release CRT)侧分配的数组 buffer 由 arx 的
		// debug operator delete 释放, 会触发 _CrtIsValidHeapPointer 跨堆断言。属 CRT 层面
		// 问题, 与绑定写法/曲线构造无关。Release 构建下已验证正常(pyge_release.log)。
		// 正式使用本 API 及 project/orthoProject/getSplitCurves/explode 请用 Release 构建。
		.def("boundBlock", [](const AcGeCurve3d& c) -> py::tuple {
			AcGeBoundBlock3d bb = c.boundBlock();
			AcGePoint3d p1, p2;
			bb.getMinMaxPoints(p1, p2);
			return py::make_tuple(p1, p2);
			})
		.def("boundBlockRange", [](const AcGeCurve3d& c, double lo, double hi) -> py::tuple {
			AcGeBoundBlock3d bb = c.boundBlock(AcGeInterval(lo, hi));
			AcGePoint3d p1, p2;
			bb.getMinMaxPoints(p1, p2);
			return py::make_tuple(p1, p2);
			}, py::arg("lower"), py::arg("upper"))
		.def("orthoBoundBlock", [](const AcGeCurve3d& c) -> py::tuple {
			AcGeBoundBlock3d bb = c.orthoBoundBlock();
			AcGePoint3d p1, p2;
			bb.getMinMaxPoints(p1, p2);
			return py::make_tuple(p1, p2);
			})
		.def("orthoBoundBlockRange", [](const AcGeCurve3d& c, double lo, double hi) -> py::tuple {
			AcGeBoundBlock3d bb = c.orthoBoundBlock(AcGeInterval(lo, hi));
			AcGePoint3d p1, p2;
			bb.getMinMaxPoints(p1, p2);
			return py::make_tuple(p1, p2);
			}, py::arg("lower"), py::arg("upper"))
		.def("project", [](const AcGeCurve3d& c, const AcGePlane& plane, const AcGeVector3d& dir, const AcGeTol& tol) {
				AcGeEntity3d* e = c.project(plane, dir, tol);
				return std::unique_ptr<AcGeEntity3d>(e);
				}, py::arg("plane"), py::arg("dir"), py::arg("tol") = AcGeContext::gTol)
			.def("orthoProject", [](const AcGeCurve3d& c, const AcGePlane& plane, const AcGeTol& tol) {
				AcGeEntity3d* e = c.orthoProject(plane, tol);
				return std::unique_ptr<AcGeEntity3d>(e);
				}, py::arg("plane"), py::arg("tol") = AcGeContext::gTol)
			.def("getSplitCurves", [](const AcGeCurve3d& c, double param) {
				AcGeCurve3d* p1 = nullptr;
				AcGeCurve3d* p2 = nullptr;
				c.getSplitCurves(param, p1, p2);
				return py::make_tuple(std::unique_ptr<AcGeCurve3d>(p1), std::unique_ptr<AcGeCurve3d>(p2));
				}, py::arg("param"))
			.def("explode", [](const AcGeCurve3d& c, py::object intrvl) {
				AcGeVoidPointerArray curves;
				AcGeIntArray newCurves;
				const AcGeInterval* iv = nullptr;
				if (!intrvl.is_none())
					iv = &intrvl.cast<const AcGeInterval&>();
				bool ok = c.explode(curves, newCurves, iv) != Adesk::kFalse;
				py::list out;
				for (int i = 0; i < (int)curves.length(); ++i)
					out.append(std::unique_ptr<AcGeCurve3d>(static_cast<AcGeCurve3d*>(curves[i])));
				py::list nc;
				for (int i = 0; i < (int)newCurves.length(); ++i)
					nc.append(newCurves[i]);
				return py::make_tuple(ok, out, nc);
				}, py::arg("intrvl") = py::none())
			.def("__repr__", [](const AcGeCurve3d&) {
			return AcString(L"AcGeCurve3d");
			});
}
