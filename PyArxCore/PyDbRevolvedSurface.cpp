#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbRevolvedSurface.h"

PyWrapper_Cpp(DbRevolvedSurface);

AcGePoint3d PyDbRevolvedSurface::getAxisPnt() const {
	return impObj()->getAxisPnt();
}

AcGeVector3d PyDbRevolvedSurface::getAxisVec() const {
	return impObj()->getAxisVec();
}

double PyDbRevolvedSurface::getRevolveAngle() const {
	return impObj()->getRevolveAngle();
}

double PyDbRevolvedSurface::getStartAngle() const {
	return impObj()->getStartAngle();
}

bool PyDbRevolvedSurface::setAxisPnt(const AcGePoint3d& pnt) {
	return PyWarnIfError(impObj()->setAxisPnt(pnt), L"AcDbRevolvedSurface::setAxisPnt");
}

bool PyDbRevolvedSurface::setAxisVec(const AcGeVector3d& vec) {
	return PyWarnIfError(impObj()->setAxisVec(vec), L"AcDbRevolvedSurface::setAxisVec");
}

bool PyDbRevolvedSurface::setRevolveAngle(double ang) {
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
