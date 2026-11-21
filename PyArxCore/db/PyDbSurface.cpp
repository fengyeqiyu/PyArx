#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSurface.h"

PyWrapper_Cpp(DbSurface);

std::optional<PyDbSurface> PyDbSurface::createFrom(PyDbEntity& srcEntity) {
	if (!srcEntity.impObj()) return std::nullopt;
	AcDbSurface* pNewSurface = nullptr;
	if (!PyWarnIfError(AcDbSurface::createFrom(srcEntity.impObj(), pNewSurface),
		L"AcDbSurface::createFrom") || !pNewSurface)
		return std::nullopt;
	return PyDbSurface(pNewSurface); // 包装接管：非 DB 驻留析构 delete，DB 驻留 close()
}

void bind_PyDbSurface(pym& m) {
	py::class_<PyDbSurface, PyDbEntity>(m, "PyDbSurface")
		.def(py::init<AcDbSurface*>(), py::arg("p"))
		PyWrapper_BIND(DbSurface)
		.def_static("createFrom", &PyDbSurface::createFrom, py::arg("srcEntity"))
		;
}
