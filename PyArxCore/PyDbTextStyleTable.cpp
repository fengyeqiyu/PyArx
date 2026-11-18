#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbTextStyleTable.h"

PyWrapper_Cpp(DbTextStyleTable);
bool PyDbTextStyleTable::has(const AcString& name) const {
	if (!impObj()) return false;
	return impObj()->has(name.kwszPtr());
}
PyDbSymbolTableRecord PyDbTextStyleTable::getAt(const AcString& name) const {
	if (!impObj()) return PyDbSymbolTableRecord(nullptr);
	AcDbTextStyleTable::RecordType* rec = nullptr;
	if(!PyWarnIfError(impObj()->getAt(name.kwszPtr(), rec, AcDb::kForRead), L"getAt")) return PyDbSymbolTableRecord(nullptr);
	return PyDbSymbolTableRecord(rec);
}

void bind_PyDbTextStyleTable(pym& m) {
	py::class_<PyDbTextStyleTable, PyDbSymbolTable>(m, "PyDbTextStyleTable")
		.def(py::init<AcDbTextStyleTable*>(), py::arg("p"))
		PyWrapper_BIND(DbTextStyleTable)
		.def("has", &PyDbTextStyleTable::has, py::arg("name"))
		.def("getAt", &PyDbTextStyleTable::getAt, py::arg("name"));
}
