#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbExtrudedSurface.h"

PyWrapper_Cpp(DbExtrudedSurface);

double PyDbExtrudedSurface::getHeight() const {
	if (!impObj()) return 0.0;
	return impObj()->getHeight();
}

AcGeVector3d PyDbExtrudedSurface::getSweepVec() const {
	if (!impObj()) return AcGeVector3d();
	return impObj()->getSweepVec();
}

bool PyDbExtrudedSurface::setHeight(double height) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setHeight(height), L"AcDbExtrudedSurface::setHeight");
}

bool PyDbExtrudedSurface::setSweepVec(const AcGeVector3d& sweepVec) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setSweepVec(sweepVec), L"AcDbExtrudedSurface::setSweepVec");
}

bool PyDbExtrudedSurface::isDependent() const {
	if (!impObj()) return false;
	return impObj()->isDependent();
}

void bind_PyDbExtrudedSurface(pym& m) {
	py::class_<PyDbExtrudedSurface, PyDbSurface>(m, "PyDbExtrudedSurface")
		.def(py::init<AcDbExtrudedSurface*>(), py::arg("p"))
		PyWrapper_BIND(DbExtrudedSurface)
		.def("getHeight", &PyDbExtrudedSurface::getHeight)
		.def("getSweepVec", &PyDbExtrudedSurface::getSweepVec)
		.def("setHeight", &PyDbExtrudedSurface::setHeight, py::arg("height"))
		.def("setSweepVec", &PyDbExtrudedSurface::setSweepVec, py::arg("sweepVec"))
		.def("isDependent", &PyDbExtrudedSurface::isDependent)
		;
}
