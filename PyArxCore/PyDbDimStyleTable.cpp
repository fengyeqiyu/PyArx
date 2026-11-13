#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDimStyleTable.h"

PyWrapper_Cpp(DbDimStyleTable);
bool PyDbDimStyleTable::has(const AcString& name) const {
	return impObj()->has(name.kwszPtr());
}
PyDbSymbolTableRecord PyDbDimStyleTable::getAt(const AcString& name) const {
	AcDbDimStyleTable::RecordType* rec = nullptr;
	if(!PyWarnIfError(impObj()->getAt(name.kwszPtr(), rec, AcDb::kForRead), L"getAt")) return PyDbSymbolTableRecord(nullptr);
	return PyDbSymbolTableRecord(rec);
}

void bind_PyDbDimStyleTable(pym& m) {
	py::class_<PyDbDimStyleTable, PyDbSymbolTable>(m, "PyDbDimStyleTable")
		.def(py::init<AcDbDimStyleTable*>(), py::arg("p"))
		PyWrapper_BIND(DbDimStyleTable)
		.def("has", &PyDbDimStyleTable::has, py::arg("name"))
		.def("getAt", &PyDbDimStyleTable::getAt, py::arg("name"));
}
