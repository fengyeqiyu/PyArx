#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSequenceEnd.h"

PyWrapper_Cpp(DbSequenceEnd);

void bind_PyDbSequenceEnd(pym& m) {
	py::class_<PyDbSequenceEnd, PyDbEntity>(m, "PyDbSequenceEnd")
		.def(py::init<>())
		.def(py::init<AcDbSequenceEnd*>(), py::arg("p"))
		PyWrapper_BIND(DbSequenceEnd)
		;
}
