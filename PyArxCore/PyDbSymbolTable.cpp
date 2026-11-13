#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyUtils.h"
#include "PyDbSymbolTable.h"

PyWrapper_Cpp(DbSymbolTable);

bool PyDbSymbolTable::has(const AcString& name) const {
	return impObj()->has(name.kwszPtr());
}

PyDbSymbolTableRecord PyDbSymbolTable::getAt(const AcString& name) const {
	AcDbSymbolTableRecord* rec = nullptr;
	if(!PyWarnIfError(impObj()->getAt(name.kwszPtr(), rec, AcDb::kForRead), L"SymbolTable.getAt")) return PyDbSymbolTableRecord(nullptr);
	return PyDbSymbolTableRecord(rec);
}

py::dict PyDbSymbolTable::toDict() const {
	py::dict out;
	AcDbSymbolTableIterator* pIter = nullptr;
	if(!PyWarnIfError(impObj()->newIterator(pIter), L"SymbolTable.newIterator")) return py::dict();
	pIter->start(true, true);
	for (; !pIter->done(); pIter->step(true, true)) {
		AcDbObjectId id;
		if(!PyWarnIfError(pIter->getRecordId(id), L"SymbolTable.getRecordId")) return py::dict();
		AcDbSymbolTableRecord* pRec = nullptr;
		if(!PyWarnIfError(pIter->getRecord(pRec, AcDb::kForRead), L"SymbolTable.getRecord")) return py::dict();
		AcString sName;
		if (pRec->getName(sName) == Acad::eOk) {
			uint64_t v = id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
			out[py::cast(sName)] = py::int_(v);
		}
		pRec->close();
	}
	delete pIter;
	return out;
}

py::list PyDbSymbolTable::names() const {
	py::list out;
	AcDbSymbolTableIterator* pIter = nullptr;
	if(!PyWarnIfError(impObj()->newIterator(pIter), L"SymbolTable.newIterator")) return py::list();
	pIter->start(true, true);
	for (; !pIter->done(); pIter->step(true, true)) {
		AcDbSymbolTableRecord* pRec = nullptr;
		if(!PyWarnIfError(pIter->getRecord(pRec, AcDb::kForRead), L"SymbolTable.getRecord")) return py::list();
		AcString sName;
		if (pRec->getName(sName) == Acad::eOk) out.append(py::cast(sName));
		pRec->close();
	}
	delete pIter;
	return out;
}

py::list PyDbSymbolTable::records() const {
	py::list out;
	AcDbSymbolTableIterator* pIter = nullptr;
	if(!PyWarnIfError(impObj()->newIterator(pIter), L"SymbolTable.newIterator")) return py::list();
	pIter->start(true, true);
	for (; !pIter->done(); pIter->step(true, true)) {
		AcDbSymbolTableRecord* pRec = nullptr;
		if(!PyWarnIfError(pIter->getRecord(pRec, AcDb::kForRead), L"SymbolTable.getRecord")) return py::list();
		out.append(py::cast(PyDbSymbolTableRecord(pRec)));
	}
	delete pIter;
	return out;
}

std::vector<PyDbSymbolTableRecord> PyDbSymbolTable::toVector() const {
	std::vector<PyDbSymbolTableRecord> out;
	AcDbSymbolTableIterator* pIter = nullptr;
	if(!PyWarnIfError(impObj()->newIterator(pIter), L"SymbolTable.newIterator")) return {};
	pIter->start(true, true);
	for (; !pIter->done(); pIter->step(true, true)) {
		AcDbSymbolTableRecord* pRec = nullptr;
		if(!PyWarnIfError(pIter->getRecord(pRec, AcDb::kForRead), L"SymbolTable.getRecord")) return out;
		out.emplace_back(pRec);
	}
	delete pIter;
	return out;
}

int PyDbSymbolTable::numRecords() const {
	AcDbSymbolTableIterator* pIter = nullptr;
	if(!PyWarnIfError(impObj()->newIterator(pIter), L"SymbolTable.newIterator")) return 0;
	int n = 0;
	pIter->start(true, true);
	for (; !pIter->done(); pIter->step(true, true)) ++n;
	delete pIter;
	return n;
}

uint64_t PyDbSymbolTable::getIdAt(const AcString& name) const {
	AcDbObjectId id;
	if (!PyWarnIfError(impObj()->getIdAt(name.kwszPtr(), id), L"SymbolTable.getIdAt")) return 0ULL;
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}

bool PyDbSymbolTable::add(PyDbSymbolTableRecord& rec) {
	return PyWarnIfError(impObj()->add(rec.impObj()), L"SymbolTable.add");
}

void bind_PyDbSymbolTable(pym& m) {
	py::class_<PyDbSymbolTable, PyDbObject>(m, "PyDbSymbolTable")
		.def(py::init<AcDbSymbolTable*>(), py::arg("p"))
		PyWrapper_BIND(DbSymbolTable)
		.def("has", &PyDbSymbolTable::has, py::arg("name"))
		.def("getAt", [](PyDbSymbolTable& self, const AcString& name, int mode) {
			AcDbSymbolTableRecord* rec = nullptr;
			if (self.impObj()->getAt(name.kwszPtr(), rec, static_cast<AcDb::OpenMode>(mode)) != Acad::eOk)
				return PyDbSymbolTableRecord(nullptr);
			return PyDbSymbolTableRecord(rec);
		}, py::arg("name"), py::arg("mode") = 0, "按名称打开记录；mode 0读/1写/2通知")
		.def("getIdAt", &PyDbSymbolTable::getIdAt, py::arg("name"), "按名称取记录 ObjectId，失败返回 0")
		.def("add", &PyDbSymbolTable::add, py::arg("rec"), "向符号表追加一条记录")
		.def("toDict", &PyDbSymbolTable::toDict)
		.def("names", &PyDbSymbolTable::names)
		.def("records", &PyDbSymbolTable::records, "返回所有记录列表")
		.def("numRecords", &PyDbSymbolTable::numRecords)
		.def("__iter__", [](PyDbSymbolTable& self) {
			return self.records().attr("__iter__")();
		}, "支持 for rec in table: 遍历")
		.def("__len__", &PyDbSymbolTable::numRecords, "len(table)")
		.def("__getitem__", [](PyDbSymbolTable& self, const AcString& name) {
			return self.getAt(name);
		}, py::arg("name"), "table['Layer0'] 按名称取记录");
}
