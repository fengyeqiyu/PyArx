#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbRegAppTableRecord.h"

PyWrapper_Cpp(DbRegAppTableRecord);

void bind_PyDbRegAppTableRecord(pym& m) {
	py::class_<PyDbRegAppTableRecord, PyDbSymbolTableRecord>(m, "PyDbRegAppTableRecord")
		.def(py::init<AcDbRegAppTableRecord*>(), py::arg("p"))
		PyWrapper_BIND(DbRegAppTableRecord);
}
