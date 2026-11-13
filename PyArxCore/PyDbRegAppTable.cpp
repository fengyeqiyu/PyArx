#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbRegAppTable.h"

PyWrapper_Cpp(DbRegAppTable);
bool PyDbRegAppTable::has(const AcString& name) const {
	return impObj()->has(name.kwszPtr());
}
PyDbSymbolTableRecord PyDbRegAppTable::getAt(const AcString& name) const {
	AcDbRegAppTable::RecordType* rec = nullptr;
	if(!PyWarnIfError(impObj()->getAt(name.kwszPtr(), rec, AcDb::kForRead), L"getAt")) return PyDbSymbolTableRecord(nullptr);
	return PyDbSymbolTableRecord(rec);
}

uint64_t PyDbRegAppTable::addReturnId(PyDbSymbolTableRecord& rec) {
	AcDbObjectId id;
	if (!PyWarnIfError(impObj()->add(id, static_cast<AcDbRegAppTableRecord*>(rec.impObj())), L"RegAppTable.addReturnId")) return 0ULL;
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}

void bind_PyDbRegAppTable(pym& m) {
	py::class_<PyDbRegAppTable, PyDbSymbolTable>(m, "PyDbRegAppTable")
		.def(py::init<AcDbRegAppTable*>(), py::arg("p"))
		PyWrapper_BIND(DbRegAppTable)
		.def("has", &PyDbRegAppTable::has, py::arg("name"))
		.def("getAt", &PyDbRegAppTable::getAt, py::arg("name"))
		.def("addReturnId", &PyDbRegAppTable::addReturnId, py::arg("rec"),
			"追加一条记录并返回其 ObjectId（旧 Id），失败返回 0");
}
