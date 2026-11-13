#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbUCSTable.h"

PyWrapper_Cpp(DbUCSTable);
bool PyDbUCSTable::has(const AcString& name) const {
	return impObj()->has(name.kwszPtr());
}
PyDbSymbolTableRecord PyDbUCSTable::getAt(const AcString& name) const {
	AcDbUCSTable::RecordType* rec = nullptr;
	if(!PyWarnIfError(impObj()->getAt(name.kwszPtr(), rec, AcDb::kForRead), L"getAt")) return PyDbSymbolTableRecord(nullptr);
	return PyDbSymbolTableRecord(rec);
}

void bind_PyDbUCSTable(pym& m) {
	py::class_<PyDbUCSTable, PyDbSymbolTable>(m, "PyDbUCSTable")
		.def(py::init<AcDbUCSTable*>(), py::arg("p"))
		PyWrapper_BIND(DbUCSTable)
		.def("has", &PyDbUCSTable::has, py::arg("name"))
		.def("getAt", &PyDbUCSTable::getAt, py::arg("name"));
}
