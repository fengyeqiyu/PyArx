#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDwfReference.h"

PyWrapper_Cpp(DbDwfReference);

void bind_PyDbDwfReference(pym& m) {
	py::class_<PyDbDwfReference, PyDbEntity>(m, "PyDbDwfReference")
		.def(py::init<>())
		.def(py::init<AcDbDwfReference*>(), py::arg("p"))
		PyWrapper_BIND(DbDwfReference)
		// ---- 本类无新增公开方法；subGetClassID 为 protected override，由基类虚表透出 ----
		;
}
