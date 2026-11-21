#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDwfDefinition.h"

PyWrapper_Cpp(DbDwfDefinition);

void bind_PyDbDwfDefinition(pym& m) {
	py::class_<PyDbDwfDefinition, PyDbObject>(m, "PyDbDwfDefinition")
		.def(py::init<>())
		.def(py::init<AcDbDwfDefinition*>(), py::arg("p"))
		PyWrapper_BIND(DbDwfDefinition)
		// ---- isDWFx ----
		.def("isDWFx", [](PyDbDwfDefinition& s) -> bool {
			if (!s.impObj()) return false;
			return s.impObj()->isDWFx() != Adesk::kFalse;
		})
		;
}
