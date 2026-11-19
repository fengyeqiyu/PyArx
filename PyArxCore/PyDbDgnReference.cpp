#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDgnReference.h"

PyWrapper_Cpp(DbDgnReference);

void bind_PyDbDgnReference(pym& m) {
	py::class_<PyDbDgnReference, PyDbEntity>(m, "PyDbDgnReference")
		.def(py::init<>())
		.def(py::init<AcDbDgnReference*>(), py::arg("p"))
		PyWrapper_BIND(DbDgnReference)
		// ---- setXrefDepth ----
		.def("setXrefDepth", [](PyDbDgnReference& s, int v) -> bool {
			if (!s.impObj()) return false;
			s.impObj()->setXrefDepth(v);
			return true;
		}, py::arg("v"))
		// ---- subGetClassID 为 protected override，由基类虚表透出，不重复绑 ----
		;
}
