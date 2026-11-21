#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbNurbSurface.h"
#include "PyDbCurve.h"

PyWrapper_Cpp(DbNurbSurface);

// AcGeKnotVector 无 pybind caster，手动与 std::vector<double> 互转。
static inline std::vector<double> KnotsToVec(const AcGeKnotVector& kv) {
	std::vector<double> out;
	out.reserve(kv.length());
	for (int i = 0; i < kv.length(); ++i) out.push_back(kv[i]);
	return out;
}
static inline AcGeKnotVector VecToKnots(const std::vector<double>& v) {
	AcGeKnotVector kv;
	for (double d : v) kv.append(d);
	return kv;
}

void bind_PyDbNurbSurface(pym& m) {
	py::class_<PyDbNurbSurface, PyDbSurface>(m, "PyDbNurbSurface")
		.def(py::init<AcDbNurbSurface*>(), py::arg("p"))
		.def(py::init<>())
		PyWrapper_BIND(DbNurbSurface)

		// ---- get / set 全量数据（knots 以 list[float] 透传） ----
		.def("get", [](PyDbNurbSurface& s) {
			int uDegree = 0, vDegree = 0, uNum = 0, vNum = 0;
			Adesk::Boolean rational = Adesk::kFalse;
			AcGePoint3dArray ctrlPts; AcGeDoubleArray weights;
			AcGeKnotVector uKnots, vKnots;
			if (!PyWarnIfError(s.impObj()->get(uDegree, vDegree, rational, uNum, vNum,
				ctrlPts, weights, uKnots, vKnots), L"AcDbNurbSurface::get"))
				return std::make_tuple(0, 0, false, 0, 0,
					AcGePoint3dArray(), AcGeDoubleArray(),
					std::vector<double>(), std::vector<double>());
			return std::make_tuple(uDegree, vDegree, rational == Adesk::kTrue, uNum, vNum,
				ctrlPts, weights, KnotsToVec(uKnots), KnotsToVec(vKnots));
		})
		.def("set", [](PyDbNurbSurface& s, int uDegree, int vDegree, bool rational,
			int uNumControlPoints, int vNumControlPoints,
			const AcGePoint3dArray& ctrlPts, const AcGeDoubleArray& weights,
			const std::vector<double>& uKnots, const std::vector<double>& vKnots) {
			return PyWarnIfError(s.impObj()->set(uDegree, vDegree, rational, uNumControlPoints,
				vNumControlPoints, ctrlPts, weights, VecToKnots(uKnots), VecToKnots(vKnots)),
				L"AcDbNurbSurface::set");
		}, py::arg("uDegree"), py::arg("vDegree"), py::arg("rational"),
			py::arg("uNumControlPoints"), py::arg("vNumControlPoints"),
			py::arg("ctrlPts"), py::arg("weights"), py::arg("uKnots"), py::arg("vKnots"))

		// ---- 控制点 ----
		.def("getControlPoints", [](PyDbNurbSurface& s) {
			int uCount = 0, vCount = 0; AcGePoint3dArray points;
			if (!PyWarnIfError(s.impObj()->getControlPoints(uCount, vCount, points),
				L"AcDbNurbSurface::getControlPoints"))
				return std::make_tuple(0, 0, AcGePoint3dArray());
			return std::make_tuple(uCount, vCount, points);
		})
		.def("setControlPoints", [](PyDbNurbSurface& s, int uCount, int vCount,
			const AcGePoint3dArray& points) {
			return PyWarnIfError(s.impObj()->setControlPoints(uCount, vCount, points),
				L"AcDbNurbSurface::setControlPoints");
		}, py::arg("uCount"), py::arg("vCount"), py::arg("points"))
		.def("getControlPointAt", [](PyDbNurbSurface& s, int uIndex, int vIndex) {
			AcGePoint3d pt;
			if (!PyWarnIfError(s.impObj()->getControlPointAt(uIndex, vIndex, pt),
				L"AcDbNurbSurface::getControlPointAt"))
				return AcGePoint3d();
			return pt;
		}, py::arg("uIndex"), py::arg("vIndex"))
		.def("setControlPointAt", [](PyDbNurbSurface& s, int uIndex, int vIndex,
			const AcGePoint3d& pt) {
			return PyWarnIfError(s.impObj()->setControlPointAt(uIndex, vIndex, pt),
				L"AcDbNurbSurface::setControlPointAt");
		}, py::arg("uIndex"), py::arg("vIndex"), py::arg("point"))
		.def("getNumberOfControlPointsInU", [](PyDbNurbSurface& s) {
			int n = 0;
			if (!PyWarnIfError(s.impObj()->getNumberOfControlPointsInU(n),
				L"AcDbNurbSurface::getNumberOfControlPointsInU")) return 0;
			return n;
		})
		.def("getNumberOfControlPointsInV", [](PyDbNurbSurface& s) {
			int n = 0;
			if (!PyWarnIfError(s.impObj()->getNumberOfControlPointsInV(n),
				L"AcDbNurbSurface::getNumberOfControlPointsInV")) return 0;
			return n;
		})
		.def("getWeight", [](PyDbNurbSurface& s, int uIndex, int vIndex) {
			double w = 0.0;
			if (!PyWarnIfError(s.impObj()->getWeight(uIndex, vIndex, w),
				L"AcDbNurbSurface::getWeight")) return 0.0;
			return w;
		}, py::arg("uIndex"), py::arg("vIndex"))
		.def("setWeight", [](PyDbNurbSurface& s, int uIndex, int vIndex, double weight) {
			return PyWarnIfError(s.impObj()->setWeight(uIndex, vIndex, weight),
				L"AcDbNurbSurface::setWeight");
		}, py::arg("uIndex"), py::arg("vIndex"), py::arg("weight"))

		// ---- 节点 ----
		.def("getUKnots", [](PyDbNurbSurface& s) {
			AcGeKnotVector knots;
			if (!PyWarnIfError(s.impObj()->getUKnots(knots), L"AcDbNurbSurface::getUKnots"))
				return std::vector<double>();
			return KnotsToVec(knots);
		})
		.def("getVKnots", [](PyDbNurbSurface& s) {
			AcGeKnotVector knots;
			if (!PyWarnIfError(s.impObj()->getVKnots(knots), L"AcDbNurbSurface::getVKnots"))
				return std::vector<double>();
			return KnotsToVec(knots);
		})
		.def("getNumberOfKnotsInU", [](PyDbNurbSurface& s) {
			int n = 0;
			if (!PyWarnIfError(s.impObj()->getNumberOfKnotsInU(n),
				L"AcDbNurbSurface::getNumberOfKnotsInU")) return 0;
			return n;
		})
		.def("getNumberOfKnotsInV", [](PyDbNurbSurface& s) {
			int n = 0;
			if (!PyWarnIfError(s.impObj()->getNumberOfKnotsInV(n),
				L"AcDbNurbSurface::getNumberOfKnotsInV")) return 0;
			return n;
		})

		// ---- 求值（三个重载 + 导数数组重载） ----
		.def("evaluate", [](PyDbNurbSurface& s, double u, double v) {
			AcGePoint3d pos;
			if (!PyWarnIfError(s.impObj()->evaluate(u, v, pos), L"AcDbNurbSurface::evaluate"))
				return AcGePoint3d();
			return pos;
		}, py::arg("u"), py::arg("v"))
		.def("evaluate1stDeriv", [](PyDbNurbSurface& s, double u, double v) {
			AcGePoint3d pos; AcGeVector3d du, dv;
			if (!PyWarnIfError(s.impObj()->evaluate(u, v, pos, du, dv),
				L"AcDbNurbSurface::evaluate(1st)"))
				return std::make_tuple(AcGePoint3d(), AcGeVector3d(), AcGeVector3d());
			return std::make_tuple(pos, du, dv);
		}, py::arg("u"), py::arg("v"))
		.def("evaluate2ndDeriv", [](PyDbNurbSurface& s, double u, double v) {
			AcGePoint3d pos; AcGeVector3d du, dv, duu, duv, dvv;
			if (!PyWarnIfError(s.impObj()->evaluate(u, v, pos, du, dv, duu, duv, dvv),
				L"AcDbNurbSurface::evaluate(2nd)"))
				return std::make_tuple(AcGePoint3d(), AcGeVector3d(), AcGeVector3d(),
					AcGeVector3d(), AcGeVector3d(), AcGeVector3d());
			return std::make_tuple(pos, du, dv, duu, duv, dvv);
		}, py::arg("u"), py::arg("v"))
		.def("evaluateDerivatives", [](PyDbNurbSurface& s, double u, double v, int derivDegree) {
			AcGePoint3d point; AcGeVector3dArray derivatives;
			if (!PyWarnIfError(s.impObj()->evaluate(u, v, derivDegree, point, derivatives),
				L"AcDbNurbSurface::evaluate(derivDegree)"))
				return std::make_tuple(AcGePoint3d(), AcGeVector3dArray());
			return std::make_tuple(point, derivatives);
		}, py::arg("u"), py::arg("v"), py::arg("derivDegree"))

		// ---- 次数 / 拓扑 ----
		.def("getDegreeInU", [](PyDbNurbSurface& s) {
			int d = 0;
			if (!PyWarnIfError(s.impObj()->getDegreeInU(d), L"AcDbNurbSurface::getDegreeInU")) return 0;
			return d;
		})
		.def("getDegreeInV", [](PyDbNurbSurface& s) {
			int d = 0;
			if (!PyWarnIfError(s.impObj()->getDegreeInV(d), L"AcDbNurbSurface::getDegreeInV")) return 0;
			return d;
		})
		.def("isClosedInU", [](PyDbNurbSurface& s) {
			bool b = false;
			if (!PyWarnIfError(s.impObj()->isClosedInU(b), L"AcDbNurbSurface::isClosedInU")) return false;
			return b;
		})
		.def("isClosedInV", [](PyDbNurbSurface& s) {
			bool b = false;
			if (!PyWarnIfError(s.impObj()->isClosedInV(b), L"AcDbNurbSurface::isClosedInV")) return false;
			return b;
		})
		.def("isPeriodicInU", [](PyDbNurbSurface& s) {
			bool b = false;
			if (!PyWarnIfError(s.impObj()->isPeriodicInU(b), L"AcDbNurbSurface::isPeriodicInU")) return false;
			return b;
		})
		.def("isPeriodicInV", [](PyDbNurbSurface& s) {
			bool b = false;
			if (!PyWarnIfError(s.impObj()->isPeriodicInV(b), L"AcDbNurbSurface::isPeriodicInV")) return false;
			return b;
		})
		.def("getPeriodInU", [](PyDbNurbSurface& s) {
			double p = 0.0;
			if (!PyWarnIfError(s.impObj()->getPeriodInU(p), L"AcDbNurbSurface::getPeriodInU")) return 0.0;
			return p;
		})
		.def("getPeriodInV", [](PyDbNurbSurface& s) {
			double p = 0.0;
			if (!PyWarnIfError(s.impObj()->getPeriodInV(p), L"AcDbNurbSurface::getPeriodInV")) return 0.0;
			return p;
		})
		.def("isRational", [](PyDbNurbSurface& s) {
			bool b = false;
			if (!PyWarnIfError(s.impObj()->isRational(b), L"AcDbNurbSurface::isRational")) return false;
			return b;
		})
		.def("getNumberOfSpansInU", [](PyDbNurbSurface& s) {
			int n = 0;
			if (!PyWarnIfError(s.impObj()->getNumberOfSpansInU(n),
				L"AcDbNurbSurface::getNumberOfSpansInU")) return 0;
			return n;
		})
		.def("getNumberOfSpansInV", [](PyDbNurbSurface& s) {
			int n = 0;
			if (!PyWarnIfError(s.impObj()->getNumberOfSpansInV(n),
				L"AcDbNurbSurface::getNumberOfSpansInV")) return 0;
			return n;
		})

		// ---- 查询 ----
		.def("isPlanar", [](PyDbNurbSurface& s) {
			bool b = false; AcGePoint3d pt; AcGeVector3d n;
			if (!PyWarnIfError(s.impObj()->isPlanar(b, pt, n), L"AcDbNurbSurface::isPlanar"))
				return std::make_tuple(false, AcGePoint3d(), AcGeVector3d());
			return std::make_tuple(b, pt, n);
		})
		.def("isPointOnSurface", [](PyDbNurbSurface& s, const AcGePoint3d& point) {
			bool b = false;
			if (!PyWarnIfError(s.impObj()->isPointOnSurface(point, b),
				L"AcDbNurbSurface::isPointOnSurface"))
				return false;
			return b;
		}, py::arg("point"))
		.def("getNormal", [](PyDbNurbSurface& s, double u, double v) {
			AcGeVector3d n;
			if (!PyWarnIfError(s.impObj()->getNormal(u, v, n), L"AcDbNurbSurface::getNormal"))
				return AcGeVector3d();
			return n;
		}, py::arg("u"), py::arg("v"))
		.def("getParameterOfPoint", [](PyDbNurbSurface& s, const AcGePoint3d& point) {
			double u = 0.0, v = 0.0;
			if (!PyWarnIfError(s.impObj()->getParameterOfPoint(point, u, v),
				L"AcDbNurbSurface::getParameterOfPoint"))
				return std::make_pair(0.0, 0.0);
			return std::make_pair(u, v);
		}, py::arg("point"))

		// ---- 等值线（返回 AcDbCurve 包装数组） ----
		.def("getIsolineAtU", [](PyDbNurbSurface& s, double u) {
			AcArray<AcDbCurve*> lineSegments;
			if (!PyWarnIfError(s.impObj()->getIsolineAtU(u, lineSegments),
				L"AcDbNurbSurface::getIsolineAtU"))
				return std::vector<PyDbCurve>();
			std::vector<PyDbCurve> out;
			out.reserve(lineSegments.length());
			for (int i = 0; i < lineSegments.length(); ++i)
				out.emplace_back(lineSegments[i]);
			return out;
		}, py::arg("u"))
		.def("getIsolineAtV", [](PyDbNurbSurface& s, double v) {
			AcArray<AcDbCurve*> lineSegments;
			if (!PyWarnIfError(s.impObj()->getIsolineAtV(v, lineSegments),
				L"AcDbNurbSurface::getIsolineAtV"))
				return std::vector<PyDbCurve>();
			std::vector<PyDbCurve> out;
			out.reserve(lineSegments.length());
			for (int i = 0; i < lineSegments.length(); ++i)
				out.emplace_back(lineSegments[i]);
			return out;
		}, py::arg("v"))

		// ---- 编辑 ----
		.def("rebuild", [](PyDbNurbSurface& s, int uDegree, int vDegree,
			int numUCtrlPts, int numVCtrlPts, bool bRestore) {
			return PyWarnIfError(s.impObj()->rebuild(uDegree, vDegree, numUCtrlPts, numVCtrlPts, bRestore),
				L"AcDbNurbSurface::rebuild");
		}, py::arg("uDegree"), py::arg("vDegree"), py::arg("numUCtrlPts"),
			py::arg("numVCtrlPts"), py::arg("bRestore") = false)
		.def("modifyPositionAndTangent", [](PyDbNurbSurface& s, double u, double v,
			const AcGePoint3d& point) {
			return PyWarnIfError(s.impObj()->modifyPositionAndTangent(u, v, point),
				L"AcDbNurbSurface::modifyPositionAndTangent");
		}, py::arg("u"), py::arg("v"), py::arg("point"))
		.def("modifyPositionAndTangent", [](PyDbNurbSurface& s, double u, double v,
			const AcGePoint3d& point, const AcGeVector3d& uDeriv, const AcGeVector3d& vDeriv) {
			return PyWarnIfError(s.impObj()->modifyPositionAndTangent(u, v, point, &uDeriv, &vDeriv),
				L"AcDbNurbSurface::modifyPositionAndTangent");
		}, py::arg("u"), py::arg("v"), py::arg("point"),
			py::arg("uDeriv"), py::arg("vDeriv"))
		;
}
