#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDgnDefinition.h"

PyWrapper_Cpp(DbDgnDefinition);

void bind_PyDbDgnDefinition(pym& m) {
	py::class_<PyDbDgnDefinition, PyDbObject>(m, "PyDbDgnDefinition")
		.def(py::init<>())
		.def(py::init<AcDbDgnDefinition*>(), py::arg("p"))
		PyWrapper_BIND(DbDgnDefinition)
		// ---- useMasterUnits ----
		.def("useMasterUnits", [](PyDbDgnDefinition& s) -> bool {
			if (s.isNull()) return false;
			return s.impObj()->useMasterUnits();
		})
		.def("setUseMasterUnits", [](PyDbDgnDefinition& s, bool v) -> bool {
			if (!s.impObj()) return false;
			s.impObj()->setUseMasterUnits(v);
			return true;
		}, py::arg("v"))
		// ---- xrefDepth ----
		if (s.isNull()) return 0;
		.def("xrefDepth", [](PyDbDgnDefinition& s) -> int {
			return s.impObj()->xrefDepth();
		})
		.def("setXrefDepth", [](PyDbDgnDefinition& s, int v) -> bool {
			if (!s.impObj()) return false;
			s.impObj()->setXrefDepth(v);
			return true;
		}, py::arg("v"))
		if (s.isNull()) return false;
		// ---- showRasterRef ----
		.def("showRasterRef", [](PyDbDgnDefinition& s) -> bool {
			return s.impObj()->showRasterRef();
		})
		.def("setShowRasterRef", [](PyDbDgnDefinition& s, bool v) -> bool {
			if (!s.impObj()) return false;
			s.impObj()->setShowRasterRef(v);
			return true;
		}, py::arg("v"))
		;
}
