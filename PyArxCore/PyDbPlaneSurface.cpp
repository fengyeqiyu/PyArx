#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbPlaneSurface.h"
#include "PyDbRegion.h"

PyWrapper_Cpp(DbPlaneSurface);

bool PyDbPlaneSurface::createFromRegion(PyDbRegion& region) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->createFromRegion(region.impObj()),
		L"AcDbPlaneSurface::createFromRegion");
}

bool PyDbPlaneSurface::isDependent() const {
	if (!impObj()) return false;
	return impObj()->isDependent();
}

void bind_PyDbPlaneSurface(pym& m) {
	py::class_<PyDbPlaneSurface, PyDbSurface>(m, "PyDbPlaneSurface")
		.def(py::init<>())
		.def(py::init<AcDbPlaneSurface*>(), py::arg("p"))
		PyWrapper_BIND(DbPlaneSurface)
		.def("createFromRegion", &PyDbPlaneSurface::createFromRegion, py::arg("region"))
		.def("isDependent", &PyDbPlaneSurface::isDependent)
		;
}
