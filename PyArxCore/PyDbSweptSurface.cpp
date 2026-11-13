#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSweptSurface.h"

PyWrapper_Cpp(DbSweptSurface);

double PyDbSweptSurface::getPathLength() const {
	double len = 0.0;
	if (!PyWarnIfError(impObj()->getPathLength(len), L"AcDbSweptSurface::getPathLength"))
		return 0.0;
	return len;
}

bool PyDbSweptSurface::isDependent() const {
	return impObj()->isDependent();
}

void bind_PyDbSweptSurface(pym& m) {
	py::class_<PyDbSweptSurface, PyDbSurface>(m, "PyDbSweptSurface")
		.def(py::init<AcDbSweptSurface*>(), py::arg("p"))
		PyWrapper_BIND(DbSweptSurface)
		.def("getPathLength", &PyDbSweptSurface::getPathLength)
		.def("isDependent", &PyDbSweptSurface::isDependent)
		;
}
