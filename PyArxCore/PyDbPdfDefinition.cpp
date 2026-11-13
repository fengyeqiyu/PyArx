#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbPdfDefinition.h"

PyWrapper_Cpp(DbPdfDefinition);

void bind_PyDbPdfDefinition(pym& m) {
	py::class_<PyDbPdfDefinition, PyDbObject>(m, "PyDbPdfDefinition")
		.def(py::init<AcDbPdfDefinition*>(), py::arg("p"))
		PyWrapper_BIND(DbPdfDefinition)
		// ---- 本类无新增公开方法（仅构造/析构 + 私有 getUnderlayHost override）----
		;
}
