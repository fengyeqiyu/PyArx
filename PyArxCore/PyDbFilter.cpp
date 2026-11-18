#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbFilter.h"
#include "PyRxClass.h"

PyWrapper_Cpp(DbFilter);

// AcDbFilter 自身仅一个方法：virtual AcRxClass* indexClass() const;
// CHM 清单里 "pack" 为疑误提取，已跳过。
void bind_PyDbFilter(pym& m) {
	py::class_<PyDbFilter, PyDbObject>(m, "PyDbFilter")
		.def(py::init<AcDbFilter*>(), py::arg("p"))
		PyWrapper_BIND(DbFilter)
		// ---- indexClass() -> PyRxClass（描述符，非所有权） ----
		.def("indexClass", [](const PyDbFilter& s) {
			if (!s.impObj()) return PyRxClass(nullptr, false);
			AcRxClass* c = s.impObj()->indexClass();
			return PyRxClass(c, false);
		}, "返回本 Filter 对应的 Index 的 AcRxClass 描述符")
		;
}
