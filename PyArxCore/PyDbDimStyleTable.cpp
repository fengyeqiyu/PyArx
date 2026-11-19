#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDimStyleTable.h"

PyWrapper_Cpp(DbDimStyleTable);
bool PyDbDimStyleTable::has(const AcString& name) const {
	if (!impObj()) return false;
	return impObj()->has(name.kwszPtr());
}
PyDbSymbolTableRecord* PyDbDimStyleTable::getAt(const AcString& name, int mode) const {
	if (!impObj()) return nullptr;
	AcDbDimStyleTable::RecordType* rec = nullptr;
	if(!PyWarnIfError(impObj()->getAt(name.kwszPtr(), rec, static_cast<AcDb::OpenMode>(mode)), L"getAt")) return nullptr;
	return new PyDbSymbolTableRecord(rec);
}

void bind_PyDbDimStyleTable(pym& m) {
	py::class_<PyDbDimStyleTable, PyDbSymbolTable>(m, "PyDbDimStyleTable")
		.def(py::init<>())
		.def(py::init<AcDbDimStyleTable*>(), py::arg("p"))
		PyWrapper_BIND(DbDimStyleTable)
		.def("has", &PyDbDimStyleTable::has, py::arg("name"))
		.def("getAt", &PyDbDimStyleTable::getAt, py::arg("name"), py::arg("mode") = 1);
}
