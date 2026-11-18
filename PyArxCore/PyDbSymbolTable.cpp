#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyUtils.h"
#include "PyDbSymbolTable.h"
#include "PyDbOpenCache.h"

PyWrapper_Cpp(DbSymbolTable);

bool PyDbSymbolTable::has(const AcString& name) const {
	if (!impObj()) return false;
	return impObj()->has(name.kwszPtr());
}

PyDbSymbolTableRecord PyDbSymbolTable::getAt(const AcString& name, int mode) const {
	if (!impObj()) return PyDbSymbolTableRecord(nullptr);
	AcDbObjectId id;
	if (impObj()->getIdAt(name.kwszPtr(), id) == Acad::eOk) {
		Acad::ErrorStatus cacheEs = Acad::eOk;
		AcDbObject* existing = PyDbOpenCache::Get(id, static_cast<AcDb::OpenMode>(mode), cacheEs);
		if (cacheEs == Acad::eOk && existing) {
			return PyDbSymbolTableRecord(static_cast<AcDbSymbolTableRecord*>(existing));
		}
	}

	AcDbSymbolTableRecord* rec = nullptr;
	const Acad::ErrorStatus es = impObj()->getAt(name.kwszPtr(), rec, static_cast<AcDb::OpenMode>(mode));
	if (es != Acad::eOk) {
		AcString esText = acadErrorStatusText(es);
		const char* esUtf8 = esText.utf8Ptr();
		const char* nameUtf8 = name.utf8Ptr();
		throw std::runtime_error(std::string("SymbolTable.getAt(\"") + (nameUtf8 ? nameUtf8 : "") +
			"\", mode=" + std::to_string(mode) + ") 失败: " + (esUtf8 ? esUtf8 : "") +
			"；需以可写方式打开数据库或另存副本（纯查询请显式传 kForRead=0）");
	}
	PyDbOpenCache::Track(rec, static_cast<AcDb::OpenMode>(mode));
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

py::list PyDbSymbolTable::records(int mode) const {
	(void)mode;
	py::list out;
	AcDbSymbolTableIterator* pIter = nullptr;
	if(!PyWarnIfError(impObj()->newIterator(pIter), L"SymbolTable.newIterator")) return py::list();
	pIter->start(true, true);
	for (; !pIter->done(); pIter->step(true, true)) {
		AcDbSymbolTableRecord* pRec = nullptr;
		if(!PyWarnIfError(pIter->getRecord(pRec, AcDb::kForRead), L"SymbolTable.getRecord")) return py::list();
		PyDbOpenCache::Track(pRec, AcDb::kForRead);
		out.append(py::cast(PyDbSymbolTableRecord(pRec)));
	}
	delete pIter;
	return out;
}

py::list PyDbSymbolTable::keys() const {
	return names();
}

py::list PyDbSymbolTable::values(int mode) const {
	return records(mode);
}

py::list PyDbSymbolTable::items(int mode) const {
	(void)mode;
	py::list out;
	AcDbSymbolTableIterator* pIter = nullptr;
	if(!PyWarnIfError(impObj()->newIterator(pIter), L"SymbolTable.newIterator")) return py::list();
	pIter->start(true, true);
	for (; !pIter->done(); pIter->step(true, true)) {
		AcDbSymbolTableRecord* pRec = nullptr;
		if(!PyWarnIfError(pIter->getRecord(pRec, AcDb::kForRead), L"SymbolTable.getRecord")) return py::list();
		AcString sName;
		if (pRec->getName(sName) == Acad::eOk) {
			PyDbOpenCache::Track(pRec, AcDb::kForRead);
			out.append(py::make_tuple(py::cast(sName), py::cast(PyDbSymbolTableRecord(pRec))));
		} else {
			pRec->close();
		}
	}
	delete pIter;
	return out;
}

py::object PyDbSymbolTable::get(const AcString& name, py::object defaultValue) const {
	if (!has(name)) return defaultValue;
	PyDbSymbolTableRecord rec = getAt(name, 1);
	if (!rec.impObj()) return defaultValue;
	return py::cast(rec);
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
	// 与实体入库同源的问题：裸 add() 不 close 记录时，记录保持写打开，
	// 之后 getAt(kForRead) 返回空包装、keys()/records() 迭代静默返回空列表
	//（实测：同一会话内新图层按名读不到，重启后被丢弃的旧记录才会消失）。
	AcDbSymbolTableRecord* pRec = static_cast<AcDbSymbolTableRecord*>(rec.impObj());
	if (!PyWarnIfError(impObj()->add(rec.impObj()), L"SymbolTable.add")) return false;
	// add 之后记录已入库并被表接管：关闭打开状态，避免写锁残留。
	if (pRec && !pRec->objectId().isNull() && pRec->isWriteEnabled()) pRec->close();
	return true;
}

PyDbSymbolTableRecord PyDbSymbolTable::recordAt(int index, int mode) const {
	if (!impObj()) return PyDbSymbolTableRecord(nullptr);
	const int total = numRecords();
	if (index < 0) index += total;
	if (index < 0 || index >= total) throw py::index_error("SymbolTable 索引越界");
	int i = 0;
	AcString found;
	AcDbSymbolTableIterator* pIter = nullptr;
	if (!PyWarnIfError(impObj()->newIterator(pIter), L"SymbolTable.newIterator")) return PyDbSymbolTableRecord(nullptr);
	for (pIter->start(true, true); !pIter->done(); pIter->step(true, true), ++i) {
		if (i != index) continue;
		AcDbSymbolTableRecord* pRec = nullptr;
		if (!PyWarnIfError(pIter->getRecord(pRec, AcDb::kForRead), L"SymbolTable.getRecord")) break;
		if (pRec->getName(found) != Acad::eOk) { pRec->close(); break; }
		pRec->close();
		break;
	}
	delete pIter;
	if (found.isEmpty()) throw py::index_error("SymbolTable 索引越界");
	return getAt(found, mode);
}

AcString PyDbSymbolTable::keyAt(int index) const {
	if (!impObj()) return AcString();
	const int total = numRecords();
	if (index < 0) index += total;
	if (index < 0 || index >= total) throw py::index_error("SymbolTable 索引越界");
	int i = 0;
	AcString found;
	AcDbSymbolTableIterator* pIter = nullptr;
	if (!PyWarnIfError(impObj()->newIterator(pIter), L"SymbolTable.newIterator")) return AcString();
	for (pIter->start(true, true); !pIter->done(); pIter->step(true, true), ++i) {
		if (i != index) continue;
		AcDbSymbolTableRecord* pRec = nullptr;
		if (!PyWarnIfError(pIter->getRecord(pRec, AcDb::kForRead), L"SymbolTable.getRecord")) break;
		if (pRec->getName(found) != Acad::eOk) found = AcString();
		pRec->close();
		break;
	}
	delete pIter;
	return found;
}

PyDbSymbolTableRecord PyDbSymbolTable::getOrCreate(const AcString& name, PyDbSymbolTableRecord& rec, int mode) {
	if (!impObj()) return PyDbSymbolTableRecord(nullptr);
	if (has(name)) return getAt(name, mode);
	AcDbSymbolTableRecord* pRec = rec.impObj();
	if (!pRec) throw std::runtime_error("SymbolTable.getOrCreate: 传入的新建记录为空（先用 XxxRecord.newRecord() 或构造新建）");
	if (pRec->setName(name.kwszPtr()) != Acad::eOk) throw std::runtime_error("SymbolTable.getOrCreate: 新记录 setName 失败");
	if (!PyWarnIfError(impObj()->add(pRec), L"SymbolTable.getOrCreate.add")) throw std::runtime_error("SymbolTable.getOrCreate: 追加记录失败");
	// add 之后记录入库并被表接管：关闭写状态，避免后续按名读不到或写锁残留
	if (!pRec->objectId().isNull() && pRec->isWriteEnabled()) pRec->close();
	return getAt(name, mode);
}
void bind_PyDbSymbolTable(pym& m) {
	py::class_<PyDbSymbolTable, PyDbObject>(m, "PyDbSymbolTable")
		.def(py::init<AcDbSymbolTable*>(), py::arg("p"))
		PyWrapper_BIND(DbSymbolTable)
		.def("has", &PyDbSymbolTable::has, py::arg("name"))
		.def("getAt", &PyDbSymbolTable::getAt, py::arg("name"), py::arg("mode") = 1, "按名称打开记录；mode 默认 1=写，纯查询传 0=读，2=通知；失败抛清晰 Python 异常")
		.def("getIdAt", &PyDbSymbolTable::getIdAt, py::arg("name"), "按名称取记录 ObjectId，失败返回 0")
		.def("add", &PyDbSymbolTable::add, py::arg("rec"), "向符号表追加一条记录")
		.def("toDict", &PyDbSymbolTable::toDict)
		.def("names", &PyDbSymbolTable::names)
		.def("records", &PyDbSymbolTable::records, py::arg("mode") = 1, "返回所有记录列表（默认写模式，纯查询传 0）")
		.def("keys", &PyDbSymbolTable::keys, "记录名列表")
		.def("values", &PyDbSymbolTable::values, py::arg("mode") = 1, "记录对象列表")
		.def("items", &PyDbSymbolTable::items, py::arg("mode") = 1, "(名, 记录) 列表")
		.def("get", &PyDbSymbolTable::get, py::arg("name"), py::arg("default") = py::none())
		.def("__contains__", &PyDbSymbolTable::has, py::arg("name"), "'name' in table")
		.def("numRecords", &PyDbSymbolTable::numRecords)
		.def("__iter__", [](PyDbSymbolTable& self) {
			return self.records().attr("__iter__")();
		}, "支持 for rec in table: 遍历")
		.def("__len__", &PyDbSymbolTable::numRecords, "len(table)")
		.def("__getitem__", [](PyDbSymbolTable& self, py::object key) {
			if (py::isinstance<py::int_>(key)) return self.recordAt(key.cast<int>(), 1);
			return self.getAt(key.cast<AcString>(), 1);
		}, py::arg("key"), "table[num] 按位置取（支持负索引）；table[name] 按名称取记录")
		.def("recordAt", &PyDbSymbolTable::recordAt, py::arg("index"), py::arg("mode") = 1, "按位置取记录（负索引从末尾算，越界抛 IndexError）")
		.def("keyAt", &PyDbSymbolTable::keyAt, py::arg("index"), "按位置取记录名")
		.def("getOrCreate", &PyDbSymbolTable::getOrCreate, py::arg("name"), py::arg("rec"), py::arg("mode") = 1, "取或建：名字不存在时把传入的新记录追加进表");
}
