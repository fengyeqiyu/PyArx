#include "stdafx.h"
#include "PyDbLine.h"
#include "PyErrorStatusException.h"

PyWrapper_Cpp(DbLine)

bool PyDbLine::isClosed() const { return impObj()->isClosed() == Adesk::kTrue; }
AcGePoint3d PyDbLine::startPoint() const { return impObj()->startPoint(); }
bool PyDbLine::setStartPoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setStartPoint(p), L"AcDbLine::setStartPoint"); }
AcGePoint3d PyDbLine::endPoint() const { return impObj()->endPoint(); }
bool PyDbLine::setEndPoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setEndPoint(p), L"AcDbLine::setEndPoint"); }
AcString PyDbLine::toString() const {
	AcGePoint3d start = startPoint(); AcGePoint3d end = endPoint();
	return AcString(std::format("AcDbLine[start=({:.2f},{:.2f},{:.2f}), end=({:.2f},{:.2f},{:.2f})]",
		start.x, start.y, start.z, end.x, end.y, end.z).c_str(), AcString::Utf8);
}
Adesk::UInt16 PyDbLine::colorIndex() const { return impObj()->colorIndex(); }
void PyDbLine::setColorIndex(Adesk::UInt16 c) { impObj()->setColorIndex(c); }

AcGeVector3d PyDbLine::normal() const { return impObj()->normal(); }
bool PyDbLine::setNormal(const AcGeVector3d& n) { return PyWarnIfError(impObj()->setNormal(n), L"AcDbLine::setNormal"); }
double PyDbLine::thickness() const { return impObj()->thickness(); }
bool PyDbLine::setThickness(double t) { return PyWarnIfError(impObj()->setThickness(t), L"AcDbLine::setThickness"); }

// 按指定平面法向偏移直线，返回新建曲线列表（未入库，需 appendEntity 入库，否则内存泄漏）
std::vector<PyDbEntity> PyDbLine::getOffsetCurvesGivenPlaneNormal(const AcGeVector3d& nrm, double offsetDist) const {
	std::vector<PyDbEntity> out;
	AcDbVoidPtrArray arr;
	if (impObj()->getOffsetCurvesGivenPlaneNormal(nrm, offsetDist, arr) != Acad::eOk) return out;
	for (int i = 0; i < arr.length(); ++i)
		out.emplace_back(static_cast<AcDbEntity*>(arr[i]));
	return out;
}

void bind_PyDbLine(pym& m) {
	py::class_<PyDbLine, PyDbCurve>(m, "PyDbLine")
		.def(py::init<>())
		.def(py::init<AcDbLine*>(), py::arg("object"))
		.def(py::init<const AcGePoint3d&, const AcGePoint3d&>(), py::arg("start"), py::arg("end"))
		.def("startPoint", &PyDbLine::startPoint)
		.def("setStartPoint", &PyDbLine::setStartPoint, py::arg("p"))
		.def("endPoint", &PyDbLine::endPoint)
		.def("setEndPoint", &PyDbLine::setEndPoint, py::arg("p"))
		.def("colorIndex", &PyDbLine::colorIndex)
		.def("setColorIndex", &PyDbLine::setColorIndex, py::arg("color"))
		.def("isClosed", &PyDbLine::isClosed)
		.def("normal", &PyDbLine::normal)
		.def("setNormal", &PyDbLine::setNormal, py::arg("n"))
		.def("thickness", &PyDbLine::thickness)
		.def("setThickness", &PyDbLine::setThickness, py::arg("t"))
		.def("getOffsetCurvesGivenPlaneNormal", &PyDbLine::getOffsetCurvesGivenPlaneNormal, py::arg("normal"), py::arg("offset_dist"), "按平面法向偏移直线，返回新曲线列表（未入库，需 appendEntity）")
		PyWrapper_BIND(DbLine)
		.def("__repr__", &PyDbLine::toString)
		;
}
