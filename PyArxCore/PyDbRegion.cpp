#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbRegion.h"

PyWrapper_Cpp(DbRegion)
AcString PyDbRegion::toString() const { return AcString(std::format("AcDbRegion[area={:.3f}]", this->area()).c_str(), AcString::Utf8); }
double PyDbRegion::area() const { double v = 0.0; if(!impObj()) return 0.0; if(!PyWarnIfError(impObj()->getArea(v), L"AcDbRegion::getArea")) return 0.0; return v; }
double PyDbRegion::perimeter() const { double v = 0.0; if(!impObj()) return 0.0; if(!PyWarnIfError(impObj()->getPerimeter(v), L"AcDbRegion::getPerimeter")) return 0.0; return v; }
AcGeVector3d PyDbRegion::normal() const { AcGeVector3d n; if(!impObj()) return AcGeVector3d(); if(!PyWarnIfError(impObj()->getNormal(n), L"AcDbRegion::getNormal")) return AcGeVector3d(); return n; }
bool PyDbRegion::isNull() const { if (!impObj()) return false; return impObj() ? (impObj()->isNull() != Adesk::kFalse) : true; }
unsigned int PyDbRegion::numChanges() const { if (!impObj()) return 0; return impObj() ? impObj()->numChanges() : 0u; }
std::tuple<AcGePlane, int> PyDbRegion::getPlane() const {
	AcGePlane plane; AcDb::Planarity type;
	if(!impObj()) return { AcGePlane(), 0 };
	if(!PyWarnIfError(impObj()->getPlane(plane, type), L"AcDbRegion::getPlane")) return { AcGePlane(), 0 };
	return { plane, static_cast<int>(type) };
}
bool PyDbRegion::booleanOper(int operation, PyDbRegion& other) {
	if (!impObj()) return false;
	if(!impObj() || !other.impObj()) return false;
	return PyWarnIfError(impObj()->booleanOper(static_cast<AcDb::BoolOperType>(operation), other.impObj()), L"AcDbRegion::booleanOper");
}
bool PyDbRegion::usesGraphicsCache() const { if (!impObj()) return false; return impObj() ? impObj()->usesGraphicsCache() : false; }
std::tuple<AcGePoint2d, double, double,
	double, double, double,
	double, double,
	AcGeVector2d, AcGeVector2d,
	double, double,
	AcGePoint2d, AcGePoint2d> PyDbRegion::getAreaProp() const {
	double perimeter = 0.0, area = 0.0, prodInertia = 0.0;
	double momInertia[2] = {0.0,0.0}, prinMoments[2] = {0.0,0.0}, radiiGyration[2] = {0.0,0.0};
	AcGePoint2d centroid, extentsLow, extentsHigh;
	AcGeVector2d prinAxes[2];
	if(!impObj()) return {};
	if(!PyWarnIfError(impObj()->getAreaProp(AcGePoint3d::kOrigin, AcGeVector3d::kXAxis, AcGeVector3d::kYAxis,
		perimeter, area, centroid, momInertia, prodInertia, prinMoments, prinAxes,
		radiiGyration, extentsLow, extentsHigh), L"AcDbRegion::getAreaProp")) return {};
	return { centroid, area, perimeter,
		momInertia[0], momInertia[1], prodInertia,
		prinMoments[0], prinMoments[1], prinAxes[0], prinAxes[1],
		radiiGyration[0], radiiGyration[1], extentsLow, extentsHigh };
}
std::vector<PyDbRegion> PyDbRegion::createFromCurves(const std::vector<PyDbCurve*>& curves) {
	AcDbVoidPtrArray in;
	for (auto* c : curves) if (c) in.append(c->impObj());
	AcDbVoidPtrArray out;
	if(!PyWarnIfError(AcDbRegion::createFromCurves(in, out), L"AcDbRegion::createFromCurves")) return {};
	std::vector<PyDbRegion> result;
	result.reserve(out.length());
	for (int i = 0; i < out.length(); ++i) result.emplace_back(static_cast<AcDbRegion*>(out[i]));
	return result;
}

void bind_PyDbRegion(pym& m) {
	py::class_<PyDbRegion, PyDbEntity>(m, "PyDbRegion")
		.def(py::init<>())
		.def(py::init<AcDbRegion*>(), py::arg("object"))
		.def("area", &PyDbRegion::area)
		.def("perimeter", &PyDbRegion::perimeter)
		.def("normal", &PyDbRegion::normal)
		.def("isNull", &PyDbRegion::isNull)
		.def("numChanges", &PyDbRegion::numChanges)
		.def("getAreaProp", &PyDbRegion::getAreaProp)
		.def("getPlane", &PyDbRegion::getPlane)
		.def("booleanOper", &PyDbRegion::booleanOper, py::arg("operation"), py::arg("otherRegion"))
		.def("usesGraphicsCache", &PyDbRegion::usesGraphicsCache)
		.def_static("createFromCurves", &PyDbRegion::createFromCurves, py::arg("curves"))
		PyWrapper_BIND(DbRegion)
		.def("__repr__", &PyDbRegion::toString)
		;
}
