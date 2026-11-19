#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLinetypeTable.h"

PyWrapper_Cpp(DbLinetypeTable);
bool PyDbLinetypeTable::has(const AcString& name) const {
	if (!impObj()) return false;
	return impObj()->has(name.kwszPtr());
}
PyDbSymbolTableRecord* PyDbLinetypeTable::getAt(const AcString& name, int mode) const {
	if (!impObj()) return nullptr;
	AcDbLinetypeTable::RecordType* rec = nullptr;
	if(!PyWarnIfError(impObj()->getAt(name.kwszPtr(), rec, static_cast<AcDb::OpenMode>(mode)), L"getAt")) return nullptr;
	return new PyDbSymbolTableRecord(rec);
}

void bind_PyDbLinetypeTable(pym& m) {
	py::class_<PyDbLinetypeTable, PyDbSymbolTable>(m, "PyDbLinetypeTable")
		.def(py::init<AcDbLinetypeTable*>(), py::arg("p"))
		PyWrapper_BIND(DbLinetypeTable)
		.def("has", &PyDbLinetypeTable::has, py::arg("name"))
		.def("getAt", &PyDbLinetypeTable::getAt, py::arg("name"), py::arg("mode") = 1);
}
