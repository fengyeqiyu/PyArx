#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSpline.h"

PyWrapper_Cpp(DbSpline)
AcString PyDbSpline::toString() const { return AcString(std::format("AcDbSpline[wrapper]").c_str(), AcString::Utf8); }
int PyDbSpline::numControlPoints() const { if (!impObj()) return 0; return impObj()->numControlPoints(); }

int PyDbSpline::degree() const { if (!impObj()) return 0; return impObj()->degree(); }

bool PyDbSpline::isNull() const { if (!impObj()) return false; return impObj()->isNull() != Adesk::kFalse; }
bool PyDbSpline::isRational() const { if (!impObj()) return false; return impObj()->isRational() != Adesk::kFalse; }
bool  PyDbSpline::elevateDegree(int newDegree) { if (!impObj()) return false; return PyWarnIfError(impObj()->elevateDegree(newDegree), L"AcDbSpline::elevateDegree"); }
AcGePoint3d PyDbSpline::getControlPointAt(int index) const { if (!impObj()) return AcGePoint3d(); AcGePoint3d p; if(!PyWarnIfError(impObj()->getControlPointAt(index, p), L"AcDbSpline::getControlPointAt")) return AcGePoint3d(); return p; }
bool  PyDbSpline::setControlPointAt(int index, const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setControlPointAt(index, p), L"AcDbSpline::setControlPointAt"); }
int PyDbSpline::numFitPoints() const { if (!impObj()) return 0; return impObj()->numFitPoints(); }
AcGePoint3d PyDbSpline::getFitPointAt(int index) const { if (!impObj()) return AcGePoint3d(); AcGePoint3d p; if(!PyWarnIfError(impObj()->getFitPointAt(index, p), L"AcDbSpline::getFitPointAt")) return AcGePoint3d(); return p; }
bool  PyDbSpline::setFitPointAt(int index, const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setFitPointAt(index, p), L"AcDbSpline::setFitPointAt"); }
bool  PyDbSpline::insertFitPointAt(int index, const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->insertFitPointAt(index, p), L"AcDbSpline::insertFitPointAt"); }
bool  PyDbSpline::removeFitPointAt(int index) { if (!impObj()) return false; return PyWarnIfError(impObj()->removeFitPointAt(index), L"AcDbSpline::removeFitPointAt"); }
double PyDbSpline::fitTolerance() const { if (!impObj()) return 0.0; return impObj()->fitTolerance(); }
bool  PyDbSpline::setFitTol(double tol) { if (!impObj()) return false; return PyWarnIfError(impObj()->setFitTol(tol), L"AcDbSpline::setFitTol"); }
double PyDbSpline::weightAt(int index) const { if (!impObj()) return 0.0; return impObj()->weightAt(index); }
bool  PyDbSpline::setWeightAt(int index, double weight) { if (!impObj()) return false; return PyWarnIfError(impObj()->setWeightAt(index, weight), L"AcDbSpline::setWeightAt"); }
bool  PyDbSpline::insertKnot(double param) { if (!impObj()) return false; return PyWarnIfError(impObj()->insertKnot(param), L"AcDbSpline::insertKnot"); }
py::tuple PyDbSpline::getFitTangents() const { if (!impObj()) return py::tuple(); AcGeVector3d s, e; if(!PyWarnIfError(impObj()->getFitTangents(s, e), L"AcDbSpline::getFitTangents")) return py::tuple(); return py::make_tuple(s, e); }
bool  PyDbSpline::setFitTangents(const AcGeVector3d& s, const AcGeVector3d& e) { if (!impObj()) return false; return PyWarnIfError(impObj()->setFitTangents(s, e), L"AcDbSpline::setFitTangents"); }
bool PyDbSpline::hasFitData() const { if (!impObj()) return false; return impObj()->hasFitData() != Adesk::kFalse; }
bool  PyDbSpline::purgeFitData() { if (!impObj()) return false; return PyWarnIfError(impObj()->purgeFitData(), L"AcDbSpline::purgeFitData"); }
bool  PyDbSpline::updateFitData() { if (!impObj()) return false; return PyWarnIfError(impObj()->updateFitData(), L"AcDbSpline::updateFitData"); }

std::tuple<AcGePoint3dArray, bool, AcGeVector3d, AcGeVector3d, int, int, double> PyDbSpline::getFitData() const {
	if (!impObj()) return std::make_tuple(AcGePoint3dArray(), false, AcGeVector3d(), AcGeVector3d(), 0, 0, 0.0);
	AcGePoint3dArray fitPoints;
	Adesk::Boolean tangentsExist = Adesk::kFalse;
	AcGeVector3d startTangent, endTangent;
	AcGe::KnotParameterization knotParam = AcGe::kChord;
	int degree = 0;
	double fitTol = 0.0;
	if (!PyWarnIfError(impObj()->getFitData(fitPoints, tangentsExist, startTangent, endTangent, knotParam, degree, fitTol), L"AcDbSpline::getFitData")) {
		return { AcGePoint3dArray(), false, AcGeVector3d(), AcGeVector3d(), 0, 0, 0.0 };
	}
	return { fitPoints, tangentsExist == Adesk::kTrue, startTangent, endTangent, static_cast<int>(knotParam), degree, fitTol };
}

std::tuple<int, bool, bool, bool, AcGePoint3dArray, AcGeDoubleArray, AcGeDoubleArray, double, double> PyDbSpline::getNurbsData() const {
	if (!impObj()) return std::make_tuple(0, false, false, false, AcGePoint3dArray(), AcGeDoubleArray(), AcGeDoubleArray(), 0.0, 0.0);
	int degree = 0;
	Adesk::Boolean rational = Adesk::kFalse, closed = Adesk::kFalse, periodic = Adesk::kFalse;
	AcGePoint3dArray ctrlPts;
	AcGeDoubleArray knots, weights;
	double ctrlPtTol = 0.0, knotTol = 0.0;
	if (!PyWarnIfError(impObj()->getNurbsData(degree, rational, closed, periodic, ctrlPts, knots, weights, ctrlPtTol, knotTol), L"AcDbSpline::getNurbsData")) {
		return { 0, false, false, false, AcGePoint3dArray(), AcGeDoubleArray(), AcGeDoubleArray(), 0.0, 0.0 };
	}
	return { degree, rational == Adesk::kTrue, closed == Adesk::kTrue, periodic == Adesk::kTrue, ctrlPts, knots, weights, ctrlPtTol, knotTol };
}

bool PyDbSpline::setFitData(const AcGePoint3dArray& fitPoints, const AcGeVector3d& startTangent, const AcGeVector3d& endTangent, int knotParam, int degree, double fitTolerance) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setFitData(fitPoints, startTangent, endTangent, static_cast<AcGe::KnotParameterization>(knotParam), degree, fitTolerance), L"AcDbSpline::setFitData");
}

// 构造器：按拟合点 + 端点切线 + 节点参数化
PyDbSpline::PyDbSpline(const AcGePoint3dArray& fitPoints, const AcGeVector3d& startTangent, const AcGeVector3d& endTangent, int knotParam, int degree, double fitTolerance)
	: PyDbCurve(new AcDbSpline(fitPoints, startTangent, endTangent, static_cast<AcGe::KnotParameterization>(knotParam), degree, fitTolerance)) {}

// 构造器：按控制点 + 节点 + 权重的 NURBS 数据
PyDbSpline::PyDbSpline(int degree, bool rational, bool closed, bool periodic, const AcGePoint3dArray& controlPoints, const AcGeDoubleArray& knots, const AcGeDoubleArray& weights, double controlPtTol, double knotTol)
	: PyDbCurve(new AcDbSpline(degree,
		rational ? Adesk::kTrue : Adesk::kFalse,
		closed ? Adesk::kTrue : Adesk::kFalse,
		periodic ? Adesk::kTrue : Adesk::kFalse,
		controlPoints, knots, weights, controlPtTol, knotTol)) {}

bool PyDbSpline::setNurbsData(int degree, bool rational, bool closed, bool periodic, const AcGePoint3dArray& controlPoints, const AcGeDoubleArray& knots, const AcGeDoubleArray& weights, double controlPtTol, double knotTol) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setNurbsData(degree,
		rational ? Adesk::kTrue : Adesk::kFalse,
		closed ? Adesk::kTrue : Adesk::kFalse,
		periodic ? Adesk::kTrue : Adesk::kFalse,
		controlPoints, knots, weights, controlPtTol, knotTol), L"AcDbSpline::setNurbsData");
}

// 样条近似为折线（新实体未入库，需 appendEntity）；maxSuggestedPrecision 出参省略
PyDbCurve PyDbSpline::toPolyline(int precision, unsigned int numOfVerticesThreshold) const {
	if (!impObj()) return PyDbCurve(nullptr);
	AcDbCurve* pCurve = nullptr;
	if (!PyWarnIfError(impObj()->toPolyline(pCurve, precision, nullptr, numOfVerticesThreshold), L"AcDbSpline::toPolyline")) return PyDbCurve(nullptr);
	return PyDbCurve(pCurve);
}

PyDbCurve PyDbSpline::toPolylineAs(bool bConvertAsArcs, bool bToLWPolyline, int precision) const {
	if (!impObj()) return PyDbCurve(nullptr);
	AcDbCurve* pCurve = nullptr;
	if (!PyWarnIfError(impObj()->toPolyline(pCurve,
		bConvertAsArcs ? Adesk::kTrue : Adesk::kFalse,
		bToLWPolyline ? Adesk::kTrue : Adesk::kFalse,
		precision, nullptr, kPlineVerticesThrehold), L"AcDbSpline::toPolylineAs")) return PyDbCurve(nullptr);
	return PyDbCurve(pCurve);
}

bool PyDbSpline::insertControlPointAt(double knotParam, const AcGePoint3d& ctrlPt, double weight) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->insertControlPointAt(knotParam, ctrlPt, weight), L"AcDbSpline::insertControlPointAt");
}

bool PyDbSpline::removeControlPointAt(int index) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->removeControlPointAt(index), L"AcDbSpline::removeControlPointAt");
}

void bind_PyDbSpline(pym& m) {
	py::class_<PyDbSpline, PyDbCurve>(m, "PyDbSpline")
		.def(py::init<>())
		.def(py::init<AcDbSpline*>(), py::arg("object"))
		.def(py::init<const AcGePoint3dArray&, const AcGeVector3d&, const AcGeVector3d&, int, int, double>(),
			py::arg("fitPoints"), py::arg("startTangent"), py::arg("endTangent"), py::arg("knotParam"),
			py::arg("degree") = 3, py::arg("fitTolerance") = 0.0)
		.def(py::init<int, bool, bool, bool, const AcGePoint3dArray&, const AcGeDoubleArray&, const AcGeDoubleArray&, double, double>(),
			py::arg("degree"), py::arg("rational"), py::arg("closed"), py::arg("periodic"),
			py::arg("controlPoints"), py::arg("knots"), py::arg("weights"),
			py::arg("controlPtTol") = 0.0, py::arg("knotTol") = AcGeKnotVector::globalKnotTolerance)
		.def("numControlPoints", &PyDbSpline::numControlPoints)
		.def("degree", &PyDbSpline::degree)
		.def("isNull", &PyDbSpline::isNull)
		.def("isRational", &PyDbSpline::isRational)
		.def("elevateDegree", &PyDbSpline::elevateDegree, py::arg("newDegree"))
		.def("getControlPointAt", &PyDbSpline::getControlPointAt, py::arg("index"))
		.def("setControlPointAt", &PyDbSpline::setControlPointAt, py::arg("index"), py::arg("point"))
		.def("numFitPoints", &PyDbSpline::numFitPoints)
		.def("getFitPointAt", &PyDbSpline::getFitPointAt, py::arg("index"))
		.def("setFitPointAt", &PyDbSpline::setFitPointAt, py::arg("index"), py::arg("point"))
		.def("insertFitPointAt", &PyDbSpline::insertFitPointAt, py::arg("index"), py::arg("point"))
		.def("removeFitPointAt", &PyDbSpline::removeFitPointAt, py::arg("index"))
		.def("fitTolerance", &PyDbSpline::fitTolerance)
		.def("setFitTol", &PyDbSpline::setFitTol, py::arg("tol"))
		.def("weightAt", &PyDbSpline::weightAt, py::arg("index"))
		.def("setWeightAt", &PyDbSpline::setWeightAt, py::arg("index"), py::arg("weight"))
		.def("insertKnot", &PyDbSpline::insertKnot, py::arg("param"))
		.def("getFitTangents", &PyDbSpline::getFitTangents)
		.def("setFitTangents", &PyDbSpline::setFitTangents, py::arg("startTangent"), py::arg("endTangent"))
		.def("hasFitData", &PyDbSpline::hasFitData)
		.def("purgeFitData", &PyDbSpline::purgeFitData)
		.def("updateFitData", &PyDbSpline::updateFitData)
		.def("getFitData", &PyDbSpline::getFitData)
		.def("getNurbsData", &PyDbSpline::getNurbsData)
		.def("setFitData", &PyDbSpline::setFitData, py::arg("fitPoints"), py::arg("startTangent"), py::arg("endTangent"), py::arg("knotParam"), py::arg("degree") = 3, py::arg("fitTolerance") = 0.0)
		.def("setNurbsData", &PyDbSpline::setNurbsData,
			py::arg("degree"), py::arg("rational"), py::arg("closed"), py::arg("periodic"),
			py::arg("controlPoints"), py::arg("knots"), py::arg("weights"),
			py::arg("controlPtTol") = 0.0, py::arg("knotTol") = 0.0, "按 NURBS 控制点/节点/权重重建样条")
		.def("toPolyline", &PyDbSpline::toPolyline,
			py::arg("precision") = 10, py::arg("numOfVerticesThreshold") = kPlineVerticesThrehold,
			"样条近似为折线（新实体未入库，需 appendEntity）")
		.def("toPolylineAs", &PyDbSpline::toPolylineAs,
			py::arg("bConvertAsArcs"), py::arg("bToLWPolyline"), py::arg("precision") = 10,
			"按弧/直线、2d/LW 类型近似为折线（新实体未入库）")
		.def("insertControlPointAt", &PyDbSpline::insertControlPointAt,
			py::arg("knotParam"), py::arg("ctrlPt"), py::arg("weight") = 1.0, "在给定节点参数处插入控制点")
		.def("removeControlPointAt", &PyDbSpline::removeControlPointAt, py::arg("index"), "删除指定控制点")
		PyWrapper_BIND(DbSpline)
		.def("__repr__", &PyDbSpline::toString)
		;
}

