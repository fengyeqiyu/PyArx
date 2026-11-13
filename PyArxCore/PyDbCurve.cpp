#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbCurve.h"
#include "PyDbEntity.h"
#include "PyDbSpline.h"

PyWrapper_Cpp(DbCurve)

bool PyDbCurve::isClosed() const { return impObj()->isClosed() != Adesk::kFalse; }
bool PyDbCurve::isPeriodic() const { return impObj()->isPeriodic() != Adesk::kFalse; }

double PyDbCurve::getStartParam() const { double v = 0.0; if(!PyWarnIfError(impObj()->getStartParam(v), L"AcDbCurve::getStartParam")) return 0.0; return v; }
double PyDbCurve::getEndParam() const { double v = 0.0; if(!PyWarnIfError(impObj()->getEndParam(v), L"AcDbCurve::getEndParam")) return 0.0; return v; }
AcGePoint3d PyDbCurve::getStartPoint() const { AcGePoint3d v; if(!PyWarnIfError(impObj()->getStartPoint(v), L"AcDbCurve::getStartPoint")) return AcGePoint3d(); return v; }
AcGePoint3d PyDbCurve::getEndPoint() const { AcGePoint3d v; if(!PyWarnIfError(impObj()->getEndPoint(v), L"AcDbCurve::getEndPoint")) return AcGePoint3d(); return v; }

AcGePoint3d PyDbCurve::getPointAtParam(double param) const { AcGePoint3d v; if(!PyWarnIfError(impObj()->getPointAtParam(param, v), L"AcDbCurve::getPointAtParam")) return AcGePoint3d(); return v; }
double PyDbCurve::getParamAtPoint(const AcGePoint3d& pt) const { double v = 0.0; if(!PyWarnIfError(impObj()->getParamAtPoint(pt, v), L"AcDbCurve::getParamAtPoint")) return 0.0; return v; }
double PyDbCurve::getDistAtParam(double param) const { double v = 0.0; if(!PyWarnIfError(impObj()->getDistAtParam(param, v), L"AcDbCurve::getDistAtParam")) return 0.0; return v; }
double PyDbCurve::getParamAtDist(double dist) const { double v = 0.0; if(!PyWarnIfError(impObj()->getParamAtDist(dist, v), L"AcDbCurve::getParamAtDist")) return 0.0; return v; }
double PyDbCurve::getDistAtPoint(const AcGePoint3d& pt) const { double v = 0.0; if(!PyWarnIfError(impObj()->getDistAtPoint(pt, v), L"AcDbCurve::getDistAtPoint")) return 0.0; return v; }
AcGePoint3d PyDbCurve::getPointAtDist(double dist) const { AcGePoint3d v; if(!PyWarnIfError(impObj()->getPointAtDist(dist, v), L"AcDbCurve::getPointAtDist")) return AcGePoint3d(); return v; }

AcGeVector3d PyDbCurve::getFirstDeriv(double param) const { AcGeVector3d v; if(!PyWarnIfError(impObj()->getFirstDeriv(param, v), L"AcDbCurve::getFirstDeriv")) return AcGeVector3d(); return v; }
AcGeVector3d PyDbCurve::getFirstDeriv(const AcGePoint3d& pt) const { AcGeVector3d v; if(!PyWarnIfError(impObj()->getFirstDeriv(pt, v), L"AcDbCurve::getFirstDeriv")) return AcGeVector3d(); return v; }
AcGeVector3d PyDbCurve::getSecondDeriv(double param) const { AcGeVector3d v; if(!PyWarnIfError(impObj()->getSecondDeriv(param, v), L"AcDbCurve::getSecondDeriv")) return AcGeVector3d(); return v; }
AcGeVector3d PyDbCurve::getSecondDeriv(const AcGePoint3d& pt) const { AcGeVector3d v; if(!PyWarnIfError(impObj()->getSecondDeriv(pt, v), L"AcDbCurve::getSecondDeriv")) return AcGeVector3d(); return v; }

AcGePoint3d PyDbCurve::getClosestPointTo(const AcGePoint3d& pt, bool extend) const {
	AcGePoint3d v;
	if(!PyWarnIfError(impObj()->getClosestPointTo(pt, v, extend ? Adesk::kTrue : Adesk::kFalse), L"AcDbCurve::getClosestPointTo")) return AcGePoint3d();
	return v;
}

double PyDbCurve::getArea() const { double v = 0.0; if(!PyWarnIfError(impObj()->getArea(v), L"AcDbCurve::getArea")) return 0.0; return v; }
bool  PyDbCurve::reverseCurve() { return PyWarnIfError(impObj()->reverseCurve(), L"AcDbCurve::reverseCurve"); }
bool  PyDbCurve::extend(double newParam) { return PyWarnIfError(impObj()->extend(newParam), L"AcDbCurve::extend"); }

// 方向投影版最近点：direction 为投影方向(入参)，仅输出曲线上点
AcGePoint3d PyDbCurve::getClosestPointTo(const AcGePoint3d& givenPnt, const AcGeVector3d& direction, bool extend) const {
	AcGePoint3d v;
	if(!PyWarnIfError(impObj()->getClosestPointTo(givenPnt, direction, v, extend ? Adesk::kTrue : Adesk::kFalse), L"AcDbCurve::getClosestPointTo")) return AcGePoint3d();
	return v;
}

bool PyDbCurve::extendToPoint(bool extendStart, const AcGePoint3d& toPoint) {
	return PyWarnIfError(impObj()->extend(extendStart ? Adesk::kTrue : Adesk::kFalse, toPoint), L"AcDbCurve::extend");
}

std::vector<PyDbEntity> PyDbCurve::getSplitCurves(const std::vector<AcGePoint3d>& points) const {
	std::vector<PyDbEntity> out;
	AcGePoint3dArray arr;
	for (const auto& p : points) arr.append(p);
	AcDbVoidPtrArray segs;
	if (impObj()->getSplitCurves(arr, segs) != Acad::eOk) return out;
	for (int i = 0; i < segs.length(); ++i) out.emplace_back(static_cast<AcDbEntity*>(segs[i]));
	return out;
}

std::vector<PyDbEntity> PyDbCurve::getOffsetCurvesGivenPlaneNormal(const AcGeVector3d& normal, double offsetDist) const {
	std::vector<PyDbEntity> out;
	AcDbVoidPtrArray arr;
	if (impObj()->getOffsetCurvesGivenPlaneNormal(normal, offsetDist, arr) != Acad::eOk) return out;
	for (int i = 0; i < arr.length(); ++i) out.emplace_back(static_cast<AcDbEntity*>(arr[i]));
	return out;
}

void bind_PyDbCurve(pym& m) {
	py::class_<PyDbCurve, PyDbEntity>(m, "PyDbCurve")
		.def(py::init<AcDbCurve*>(), py::arg("object"))
		.def("isClosed", &PyDbCurve::isClosed)
		.def("isPeriodic", &PyDbCurve::isPeriodic)
		.def("getStartParam", &PyDbCurve::getStartParam)
		.def("getEndParam", &PyDbCurve::getEndParam)
		.def("getStartPoint", &PyDbCurve::getStartPoint)
		.def("getEndPoint", &PyDbCurve::getEndPoint)
		.def("getPointAtParam", &PyDbCurve::getPointAtParam, py::arg("param"))
		.def("getParamAtPoint", &PyDbCurve::getParamAtPoint, py::arg("pt"))
		.def("getDistAtParam", &PyDbCurve::getDistAtParam, py::arg("param"))
		.def("getParamAtDist", &PyDbCurve::getParamAtDist, py::arg("dist"))
		.def("getDistAtPoint", &PyDbCurve::getDistAtPoint, py::arg("pt"))
		.def("getPointAtDist", &PyDbCurve::getPointAtDist, py::arg("dist"))
		.def("getFirstDeriv", py::overload_cast<double>(&PyDbCurve::getFirstDeriv, py::const_))
		.def("getFirstDeriv", py::overload_cast<const AcGePoint3d&>(&PyDbCurve::getFirstDeriv, py::const_), py::arg("pt"))
		.def("getSecondDeriv", py::overload_cast<double>(&PyDbCurve::getSecondDeriv, py::const_))
		.def("getSecondDeriv", py::overload_cast<const AcGePoint3d&>(&PyDbCurve::getSecondDeriv, py::const_), py::arg("pt"))
		.def("getClosestPointTo", py::overload_cast<const AcGePoint3d&, bool>(&PyDbCurve::getClosestPointTo, py::const_), py::arg("pt"), py::arg("extend") = false)
		.def("getClosestPointTo", py::overload_cast<const AcGePoint3d&, const AcGeVector3d&, bool>(&PyDbCurve::getClosestPointTo, py::const_), py::arg("givenPnt"), py::arg("direction"), py::arg("extend") = false, "沿 direction 方向投影求最近点，返回曲线上点")
		.def("getArea", &PyDbCurve::getArea)
		.def("reverseCurve", &PyDbCurve::reverseCurve)
		.def("extend", &PyDbCurve::extend, py::arg("newParam"))
		.def("extendToPoint", &PyDbCurve::extendToPoint, py::arg("extendStart"), py::arg("toPoint"), "延伸曲线到指定点")
		// 偏移曲线：返回新建实体列表（未入库，需 appendEntity 入库，否则内存泄漏）
		.def("getOffsetCurves", [](const PyDbCurve& self, double dist) -> py::list {
			py::list out;
			AcDbVoidPtrArray arr;
			if (self.impObj()->getOffsetCurves(dist, arr) != Acad::eOk) return out;
			for (int i = 0; i < arr.length(); ++i)
				out.append(PyDbEntity(static_cast<AcDbEntity*>(arr[i])));
			return out;
		}, py::arg("offset_distance"), "偏移曲线，返回新实体列表（未入库，需 appendEntity）")
		// 按参数点分割曲线，返回新曲线列表（未入库）
		.def("getSplitCurves", [](const PyDbCurve& self, const std::vector<double>& params) -> py::list {
			py::list out;
			AcGeDoubleArray arr;
			for (double d : params) arr.append(d);
			AcDbVoidPtrArray segs;
			if (self.impObj()->getSplitCurves(arr, segs) != Acad::eOk) return out;
			for (int i = 0; i < segs.length(); ++i)
				out.append(PyDbEntity(static_cast<AcDbEntity*>(segs[i])));
			return out;
		}, py::arg("params"), "按参数点分割曲线，返回新曲线列表（未入库）")
		// 拟合为样条，返回 AcDbSpline（未入库）
		.def("getSpline", [](const PyDbCurve& self) -> PyDbSpline {
			AcDbSpline* sp = nullptr;
			if (self.impObj()->getSpline(sp) != Acad::eOk) sp = nullptr;
			return PyDbSpline(sp);
		}, "拟合为样条曲线（未入库）")
		// 按点列分割曲线，返回新曲线列表（未入库）
		.def("getSplitCurves", py::overload_cast<const std::vector<AcGePoint3d>&>(&PyDbCurve::getSplitCurves, py::const_),
			py::arg("points"), "按点列分割曲线，返回新曲线列表（未入库，需 appendEntity）")
		// 按平面法向偏移曲线，返回新曲线列表（未入库）
		.def("getOffsetCurvesGivenPlaneNormal", &PyDbCurve::getOffsetCurvesGivenPlaneNormal,
			py::arg("normal"), py::arg("offset_dist"), "按平面法向偏移，返回新曲线列表（未入库，需 appendEntity）")
		// 投影到平面，返回新曲线（未入库）
		.def("getProjectedCurve", [](const PyDbCurve& self, const AcGePlane& pl, const AcGeVector3d& projDir) -> PyDbCurve {
			AcDbCurve* c = nullptr;
			if (self.impObj()->getProjectedCurve(pl, projDir, c) != Acad::eOk) c = nullptr;
			return PyDbCurve(c);
		}, py::arg("plane"), py::arg("proj_dir"), "沿方向投影到平面，返回新曲线（未入库）")
		.def("getOrthoProjectedCurve", [](const PyDbCurve& self, const AcGePlane& pl) -> PyDbCurve {
			AcDbCurve* c = nullptr;
			if (self.impObj()->getOrthoProjectedCurve(pl, c) != Acad::eOk) c = nullptr;
			return PyDbCurve(c);
		}, py::arg("plane"), "正交投影到平面，返回新曲线（未入库）")
		// 取出对应的 GE 曲线（堆对象，返回裸指针由 Python 接管 delete）
		.def("getAcGeCurve", [](const PyDbCurve& self) -> AcGeCurve3d* {
			AcGeCurve3d* g = nullptr;
			if (self.impObj()->getAcGeCurve(g) != Acad::eOk) return nullptr;
			return g;
		}, "返回对应的 AcGeCurve3d（堆对象，Python 接管内存）")
		// 由 GE 曲线新建 AcDbCurve（静态，未入库）
		.def_static("createFromAcGeCurve", [](const AcGeCurve3d& geCurve) -> PyDbCurve {
			AcDbCurve* dbc = nullptr;
			if (AcDbCurve::createFromAcGeCurve(geCurve, dbc) != Acad::eOk) dbc = nullptr;
			return PyDbCurve(dbc);
		}, py::arg("ge_curve"), "由 AcGeCurve3d 创建 AcDbCurve（未入库，需 appendEntity）")
		// 用 GE 曲线重设当前曲线
		.def("setFromAcGeCurve", [](PyDbCurve& self, const AcGeCurve3d& geCurve) -> bool {
			return PyWarnIfError(self.impObj()->setFromAcGeCurve(geCurve), L"AcDbCurve::setFromAcGeCurve");
		}, py::arg("ge_curve"))
		PyWrapper_BIND(DbCurve)
		;
}
