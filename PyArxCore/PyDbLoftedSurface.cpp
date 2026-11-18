#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLoftedSurface.h"

PyWrapper_Cpp(DbLoftedSurface);

int PyDbLoftedSurface::numCrossSections() const {
	if (!impObj()) return 0;
	return impObj()->numCrossSections();
}

int PyDbLoftedSurface::numGuideCurves() const {
	if (!impObj()) return 0;
	return impObj()->numGuideCurves();
}

void bind_PyDbLoftedSurface(pym& m) {
	py::class_<PyDbLoftedSurface, PyDbSurface>(m, "PyDbLoftedSurface")
		.def(py::init<AcDbLoftedSurface*>(), py::arg("p"))
		PyWrapper_BIND(DbLoftedSurface)
		.def("numCrossSections", &PyDbLoftedSurface::numCrossSections)
		.def("numGuideCurves", &PyDbLoftedSurface::numGuideCurves)
		;
}
