#include "stdafx.h"
#include "PyGeCurve2d.h"
#include <geblok2d.h>
#include <format>
#include <string>

void bind_AcGeCurve2d(pybind11::module& m) {
	// Abstract base: no py::init.
	py::class_<AcGeCurve2d, AcGeEntity2d>(m, "AcGeCurve2d")
		.def("distanceTo",
			py::overload_cast<const AcGePoint2d&, const AcGeTol&>(&AcGeCurve2d::distanceTo, py::const_),
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("closestPointTo",
			py::overload_cast<const AcGePoint2d&, const AcGeTol&>(&AcGeCurve2d::closestPointTo, py::const_),
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("isClosed", &AcGeCurve2d::isClosed,
			py::arg("tol") = AcGeContext::gTol)
		.def("isOn",
			py::overload_cast<const AcGePoint2d&, const AcGeTol&>(&AcGeCurve2d::isOn, py::const_),
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("paramOf", &AcGeCurve2d::paramOf,
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("getInterval", [](const AcGeCurve2d& self) {
			AcGeInterval iv;
			self.getInterval(iv);
			return py::make_tuple(iv.lowerBound(), iv.upperBound());
			}, "Return (lowerBound, upperBound) of the parameter interval")
		.def("setInterval", [](AcGeCurve2d& self, double lo, double hi) {
			AcGeInterval iv(lo, hi);
			return self.setInterval(iv) != Adesk::kFalse;
			}, py::arg("lower"), py::arg("upper"))
		.def("evalPoint", py::overload_cast<double>(&AcGeCurve2d::evalPoint, py::const_), py::arg("param"))
		.def("length", &AcGeCurve2d::length,
			py::arg("fromParam"), py::arg("toParam"),
			py::arg("tol") = AcGeContext::gTol.equalPoint())
		.def("hasStartPoint", [](const AcGeCurve2d& self) {
			AcGePoint2d p;
			bool ok = self.hasStartPoint(p);
			return py::make_tuple(ok, p);
			})
		.def("hasEndPoint", [](const AcGeCurve2d& self) {
			AcGePoint2d p;
			bool ok = self.hasEndPoint(p);
			return py::make_tuple(ok, p);
			})
		.def("reverseParam", &AcGeCurve2d::reverseParam)
		.def("resetInterval", py::overload_cast<>(&AcGeCurve2d::setInterval))
		.def("paramAtLength", &AcGeCurve2d::paramAtLength, py::arg("datumParam"), py::arg("length"), py::arg("posParamDir") = Adesk::kTrue, py::arg("tol") = AcGeContext::gTol.equalPoint())
		.def("isPeriodic", [](const AcGeCurve2d& self) { double period = 0.0; Adesk::Boolean ok = self.isPeriodic(period); return py::make_tuple(ok != Adesk::kFalse, period); })
		.def("isOnParam", py::overload_cast<double, const AcGeTol&>(&AcGeCurve2d::isOn, py::const_), py::arg("param"), py::arg("tol") = AcGeContext::gTol)
		.def("distanceToCurve", py::overload_cast<const AcGeCurve2d&, const AcGeTol&>(&AcGeCurve2d::distanceTo, py::const_), py::arg("other"), py::arg("tol") = AcGeContext::gTol)
		.def("getSamplePoints", [](const AcGeCurve2d& c, int numSample) {
				AcGePoint2dArray pts;
				c.getSamplePoints(numSample, pts);
				return pts;
				}, py::arg("numSample"))
			.def("getSamplePointsRange", [](const AcGeCurve2d& c, double from, double to, double eps) -> py::tuple {
				AcGePoint2dArray pts;
				AcGeDoubleArray params;
				c.getSamplePoints(from, to, eps, pts, params);
				py::list pl;
				for (int i = 0; i < (int)params.length(); ++i) pl.append(params[i]);
				return py::make_tuple(pts, pl);
				}, py::arg("from"), py::arg("to"), py::arg("approxEps"))
			.def("getSplitCurves", [](const AcGeCurve2d& c, double param) {
				AcGeCurve2d* p1 = nullptr;
				AcGeCurve2d* p2 = nullptr;
				c.getSplitCurves(param, p1, p2);
				return py::make_tuple(std::unique_ptr<AcGeCurve2d>(p1), std::unique_ptr<AcGeCurve2d>(p2));
				}, py::arg("param"))
			.def("explode", [](const AcGeCurve2d& c, py::object intrvl) {
				AcGeVoidPointerArray curves;
				AcGeIntArray newCurves;
				const AcGeInterval* iv = nullptr;
				if (!intrvl.is_none())
					iv = &intrvl.cast<const AcGeInterval&>();
				bool ok = c.explode(curves, newCurves, iv) != Adesk::kFalse;
				py::list out;
				for (int i = 0; i < (int)curves.length(); ++i)
					out.append(std::unique_ptr<AcGeCurve2d>(static_cast<AcGeCurve2d*>(curves[i])));
				py::list nc;
				for (int i = 0; i < (int)newCurves.length(); ++i)
					nc.append(newCurves[i]);
				return py::make_tuple(ok, out, nc);
				}, py::arg("intrvl") = py::none())
			.def("isLinear", [](const AcGeCurve2d& c, const AcGeTol& tol) -> py::tuple {
				AcGeLine2d line;
				bool ok = c.isLinear(line, tol) != Adesk::kFalse;
				return py::make_tuple(ok, line);
				}, py::arg("tol") = AcGeContext::gTol)
			.def("isDegenerate", [](const AcGeCurve2d& c, const AcGeTol& tol) -> py::tuple {
				AcGe::EntityId type = static_cast<AcGe::EntityId>(0);
				bool ok = c.isDegenerate(type, tol) != Adesk::kFalse;
				return py::make_tuple(ok, ok ? (int)type : 0);
			}, py::arg("tol") = AcGeContext::gTol)
			.def("boundBlock", [](const AcGeCurve2d& c) -> py::tuple {
				AcGeBoundBlock2d bb = c.boundBlock();
				AcGePoint2d p1, p2;
				bb.getMinMaxPoints(p1, p2);
				return py::make_tuple(p1, p2);
			})
			.def("boundBlockRange", [](const AcGeCurve2d& c, double lo, double hi) -> py::tuple {
				AcGeBoundBlock2d bb = c.boundBlock(AcGeInterval(lo, hi));
				AcGePoint2d p1, p2;
				bb.getMinMaxPoints(p1, p2);
				return py::make_tuple(p1, p2);
			}, py::arg("lower"), py::arg("upper"))
			.def("orthoBoundBlock", [](const AcGeCurve2d& c) -> py::tuple {
				AcGeBoundBlock2d bb = c.orthoBoundBlock();
				AcGePoint2d p1, p2;
				bb.getMinMaxPoints(p1, p2);
				return py::make_tuple(p1, p2);
			})
			.def("orthoBoundBlockRange", [](const AcGeCurve2d& c, double lo, double hi) -> py::tuple {
				AcGeBoundBlock2d bb = c.orthoBoundBlock(AcGeInterval(lo, hi));
				AcGePoint2d p1, p2;
				bb.getMinMaxPoints(p1, p2);
				return py::make_tuple(p1, p2);
			}, py::arg("lower"), py::arg("upper"))
			.def("__repr__", [](const AcGeCurve2d&) {
			return AcString(L"AcGeCurve2d");
			});
}
