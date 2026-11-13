#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbBlockBegin.h"

PyWrapper_Cpp(DbBlockBegin);

void bind_PyDbBlockBegin(pym& m) {
	py::class_<PyDbBlockBegin, PyDbEntity>(m, "PyDbBlockBegin")
		.def(py::init<AcDbBlockBegin*>(), py::arg("p"))
		PyWrapper_BIND(DbBlockBegin)
		;
}
