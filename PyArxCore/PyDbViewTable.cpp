#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbViewTable.h"

PyWrapper_Cpp(DbViewTable);
bool PyDbViewTable::has(const AcString& name) const {
	if (!impObj()) return false;
	return impObj()->has(name.kwszPtr());
}
PyDbSymbolTableRecord* PyDbViewTable::getAt(const AcString& name, int mode) const {
	if (!impObj()) return nullptr;
	AcDbViewTable::RecordType* rec = nullptr;
	if(!PyWarnIfError(impObj()->getAt(name.kwszPtr(), rec, static_cast<AcDb::OpenMode>(mode)), L"getAt")) return nullptr;
	return new PyDbSymbolTableRecord(rec);
}

void bind_PyDbViewTable(pym& m) {
	py::class_<PyDbViewTable, PyDbSymbolTable>(m, "PyDbViewTable")
		.def(py::init<AcDbViewTable*>(), py::arg("p"))
		PyWrapper_BIND(DbViewTable)
		.def("has", &PyDbViewTable::has, py::arg("name"))
		.def("getAt", &PyDbViewTable::getAt, py::arg("name"), py::arg("mode") = 1);
}
