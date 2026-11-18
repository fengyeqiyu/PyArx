#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbRevolvedSurface.h"

PyWrapper_Cpp(DbRevolvedSurface);

AcGePoint3d PyDbRevolvedSurface::getAxisPnt() const {
	if (!impObj()) return AcGePoint3d();
	return impObj()->getAxisPnt();
}

AcGeVector3d PyDbRevolvedSurface::getAxisVec() const {
	if (!impObj()) return AcGeVector3d();
	return impObj()->getAxisVec();
}

double PyDbRevolvedSurface::getRevolveAngle() const {
	return impObj() ? impObj()->getRevolveAngle() : 0.0;
}

double PyDbRevolvedSurface::getStartAngle() const {
	return impObj() ? impObj()->getStartAngle() : 0.0;
}

bool PyDbRevolvedSurface::setAxisPnt(const AcGePoint3d& pnt) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setAxisPnt(pnt), L"AcDbRevolvedSurface::setAxisPnt");
}

bool PyDbRevolvedSurface::setAxisVec(const AcGeVector3d& vec) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setAxisVec(vec), L"AcDbRevolvedSurface::setAxisVec");
}

bool PyDbRevolvedSurface::setRevolveAngle(double ang) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setRevolveAngle(ang), L"AcDbRevolvedSurface::setRevolveAngle");
}

void bind_PyDbRevolvedSurface(pym& m) {
	py::class_<PyDbRevolvedSurface, PyDbSurface>(m, "PyDbRevolvedSurface")
		.def(py::init<AcDbRevolvedSurface*>(), py::arg("p"))
		PyWrapper_BIND(DbRevolvedSurface)
		.def("getAxisPnt", &PyDbRevolvedSurface::getAxisPnt)
		.def("getAxisVec", &PyDbRevolvedSurface::getAxisVec)
		.def("getRevolveAngle", &PyDbRevolvedSurface::getRevolveAngle)
		.def("getStartAngle", &PyDbRevolvedSurface::getStartAngle)
		.def("setAxisPnt", &PyDbRevolvedSurface::setAxisPnt, py::arg("pnt"))
		.def("setAxisVec", &PyDbRevolvedSurface::setAxisVec, py::arg("vec"))
		.def("setRevolveAngle", &PyDbRevolvedSurface::setRevolveAngle, py::arg("ang"))
		;
}
