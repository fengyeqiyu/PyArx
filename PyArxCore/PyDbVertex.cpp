#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbVertex.h"

PyWrapper_Cpp(DbVertex);

void bind_PyDbVertex(pym& m) {
	py::class_<PyDbVertex, PyDbEntity>(m, "PyDbVertex")
		.def(py::init<AcDbVertex*>(), py::arg("object"))
		PyWrapper_BIND(DbVertex)
		;
}
