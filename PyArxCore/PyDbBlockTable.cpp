#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbBlockTable.h"

PyWrapper_Cpp(DbBlockTable);
bool PyDbBlockTable::has(const AcString& name) const {
	if (!impObj()) return false;
	return impObj()->has(name.kwszPtr());
}
PyDbBlockTableRecord* PyDbBlockTable::getAt(const AcString& name, int mode) const {
	if (!impObj()) return nullptr;
	AcDbBlockTable::RecordType* rec = nullptr;
	if(!PyWarnIfError(impObj()->getAt(name.kwszPtr(), rec, static_cast<AcDb::OpenMode>(mode)), L"getAt")) return nullptr;
	return new PyDbBlockTableRecord(rec);
}

void bind_PyDbBlockTable(pym& m) {
	py::class_<PyDbBlockTable, PyDbSymbolTable>(m, "PyDbBlockTable")
		.def(py::init<AcDbBlockTable*>(), py::arg("p"))
		PyWrapper_BIND(DbBlockTable)
		.def("has", &PyDbBlockTable::has, py::arg("name"))
		.def("getAt", &PyDbBlockTable::getAt, py::arg("name"), py::arg("mode") = 1,
			"按名称取块表记录；mode 默认 1=写，纯查询传 0=读");
}
