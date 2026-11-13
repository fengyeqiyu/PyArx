#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLinetypeTable.h"

PyWrapper_Cpp(DbLinetypeTable);
bool PyDbLinetypeTable::has(const AcString& name) const {
	return impObj()->has(name.kwszPtr());
}
PyDbSymbolTableRecord PyDbLinetypeTable::getAt(const AcString& name) const {
	AcDbLinetypeTable::RecordType* rec = nullptr;
	if(!PyWarnIfError(impObj()->getAt(name.kwszPtr(), rec, AcDb::kForRead), L"getAt")) return PyDbSymbolTableRecord(nullptr);
	return PyDbSymbolTableRecord(rec);
}

void bind_PyDbLinetypeTable(pym& m) {
	py::class_<PyDbLinetypeTable, PyDbSymbolTable>(m, "PyDbLinetypeTable")
		.def(py::init<AcDbLinetypeTable*>(), py::arg("p"))
		PyWrapper_BIND(DbLinetypeTable)
		.def("has", &PyDbLinetypeTable::has, py::arg("name"))
		.def("getAt", &PyDbLinetypeTable::getAt, py::arg("name"));
}
