#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbViewportTable.h"

PyWrapper_Cpp(DbViewportTable);
bool PyDbViewportTable::has(const AcString& name) const {
	if (!impObj()) return false;
	return impObj()->has(name.kwszPtr());
}
PyDbSymbolTableRecord PyDbViewportTable::getAt(const AcString& name) const {
	if (!impObj()) return PyDbSymbolTableRecord(nullptr);
	AcDbViewportTable::RecordType* rec = nullptr;
	if(!PyWarnIfError(impObj()->getAt(name.kwszPtr(), rec, AcDb::kForRead), L"getAt")) return PyDbSymbolTableRecord(nullptr);
	return PyDbSymbolTableRecord(rec);
}

std::vector<uint64_t> PyDbViewportTable::newIterator(bool atBeginning, bool skipDeleted) const {
	std::vector<uint64_t> out;
	AcDbViewportTableIterator* pIter = nullptr;
	if(!PyWarnIfError(impObj()->newIterator(pIter, atBeginning, skipDeleted), L"AcDbViewportTable::newIterator")) return out;
	for (; !pIter->done(); pIter->step(true, true)) {
		AcDbObjectId id;
		if (pIter->getRecordId(id) == Acad::eOk && !id.isNull())
			out.push_back(static_cast<uint64_t>(id.asOldId()));
	}
	delete pIter;
	return out;
}

void bind_PyDbViewportTable(pym& m) {
	py::class_<PyDbViewportTable, PyDbSymbolTable>(m, "PyDbViewportTable")
		.def(py::init<AcDbViewportTable*>(), py::arg("p"))
		PyWrapper_BIND(DbViewportTable)
		.def("has", &PyDbViewportTable::has, py::arg("name"))
		.def("getAt", &PyDbViewportTable::getAt, py::arg("name"))
		.def("newIterator", &PyDbViewportTable::newIterator, py::arg("atBeginning") = true, py::arg("skipDeleted") = true, "遍历视口表，返回全部记录 ObjectId（uint64）列表；裸迭代器在 C++ 侧消费并释放");
}
