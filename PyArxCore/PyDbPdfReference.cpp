#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbPdfReference.h"

PyWrapper_Cpp(DbPdfReference);

void bind_PyDbPdfReference(pym& m) {
	py::class_<PyDbPdfReference, PyDbEntity>(m, "PyDbPdfReference")
		.def(py::init<AcDbPdfReference*>(), py::arg("p"))
		PyWrapper_BIND(DbPdfReference)
		// ---- contrastDefault / fadeDefault（静态，继承自 AcDbUnderlayReference）----
		.def_static("contrastDefault", []() -> int {
			return static_cast<int>(AcDbUnderlayReference::contrastDefault());
		})
		.def_static("fadeDefault", []() -> int {
			return static_cast<int>(AcDbUnderlayReference::fadeDefault());
		})
		// ---- subGetClassID 为 protected override，由基类虚表透出，不重复绑 ----
		;
}
