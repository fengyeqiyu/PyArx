#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbBlockEnd.h"

PyWrapper_Cpp(DbBlockEnd);

void bind_PyDbBlockEnd(pym& m) {
	py::class_<PyDbBlockEnd, PyDbEntity>(m, "PyDbBlockEnd")
		.def(py::init<AcDbBlockEnd*>(), py::arg("p"))
		PyWrapper_BIND(DbBlockEnd)
		;
}
