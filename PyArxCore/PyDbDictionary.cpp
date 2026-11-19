#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyUtils.h"
#include "PyDbDictionary.h"
#include "PyDbTrans.h"

PyWrapper_Cpp(DbDictionary);

bool  PyDbDictionary::add(const AcString& name, PyDbObject& obj) {
	if (!impObj()) return false;
	AcDbObjectId retId;
	if (!PyWarnIfError(impObj()->setAt(name.kwszPtr(), obj.impObj(), retId), L"Dictionary.add")) return false;
	// 归属权已转移给字典/数据库：包装释放所有权，否则 Python 侧析构 delete 已入库对象 → 崩溃。
	obj.detachDbObject();
	return true;
}

py::object PyDbDictionary::getAt(const AcString& name) {
	if (!impObj()) return py::none();
	AcDbObject* p = nullptr;
	if (!PyWarnIfError(impObj()->getAt(name.kwszPtr(), p, AcDb::kForRead), L"Dictionary.getAt")) return py::none();
	return pyarxWrapDbObjectByClass(p);
}

bool  PyDbDictionary::remove(const AcString& name) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->remove(name.kwszPtr()), L"Dictionary.remove");
}

int PyDbDictionary::numEntries() const {
	if (!impObj()) return 0;
	return static_cast<int>(impObj()->numEntries());
}

bool PyDbDictionary::has(const AcString& name) const {
	if (!impObj()) return false;
	return impObj()->has(name.kwszPtr());
}

uint64_t PyDbDictionary::getIdAt(const AcString& name) const {
	if (!impObj()) return 0ULL;
	AcDbObjectId id;
	if (!PyWarnIfError(impObj()->getAt(name.kwszPtr(), id), L"Dictionary.getIdAt")) return 0ULL;
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}

AcString PyDbDictionary::nameAt(uint64_t objId) const {
	if (!impObj()) return AcString();
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(objId));
	AcString s;
	if (!PyWarnIfError(impObj()->nameAt(id, s), L"Dictionary.nameAt")) return AcString();
	return s;
}

bool PyDbDictionary::removeById(uint64_t objId) {
	if (!impObj()) return false;
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(objId));
	return PyWarnIfError(impObj()->remove(id), L"Dictionary.removeById");
}

bool PyDbDictionary::hasId(uint64_t objId) const {
	if (!impObj()) return false;
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(objId));
	return impObj()->has(id);
}

bool PyDbDictionary::rename(const AcString& oldName, const AcString& newName) {
	if (!impObj()) return false;
	return impObj()->setName(oldName.kwszPtr(), newName.kwszPtr());
}

std::tuple<bool, uint64_t> PyDbDictionary::setAt(const AcString& key, PyDbObject& obj) {
	if (!impObj()) return {};
	AcDbObjectId retId;
	bool ok = PyWarnIfError(impObj()->setAt(key.kwszPtr(), obj.impObj(), retId), L"Dictionary.setAt");
	uint64_t idv = (ok && !retId.isNull()) ? static_cast<uint64_t>(retId.asOldId()) : 0ULL;
	return std::make_tuple(ok, idv);
}

AcDb::DuplicateRecordCloning PyDbDictionary::mergeStyle() const {
	if (!impObj()) return {};
	return impObj()->mergeStyle();
}

void PyDbDictionary::setMergeStyle(int style) {
	if (!impObj()) return;
	impObj()->setMergeStyle(static_cast<AcDb::DuplicateRecordCloning>(style));
}

bool PyDbDictionary::isTreatElementsAsHard() const {
	if (!impObj()) return false;
	return impObj()->isTreatElementsAsHard();
}

void PyDbDictionary::setTreatElementsAsHard(bool doIt) {
	if (!impObj()) return;
	impObj()->setTreatElementsAsHard(doIt);
}

py::dict PyDbDictionary::toDict() {
	if (!impObj()) return py::dict();
	py::dict out;
	AcDbDictionaryIterator* pIter = impObj()->newIterator();
	if (pIter == nullptr) return out;
	for (; !pIter->done(); pIter->next()) {
		AcDbObjectId id = pIter->objectId();
		uint64_t v = id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		AcString sName;
		if (pIter->name(sName)) out[py::cast(sName)] = py::int_(v);
	}
	delete pIter;
	return out;
}

py::list PyDbDictionary::names() const {
	py::list out;
	if (!impObj()) return out;
	AcDbDictionaryIterator* pIter = impObj()->newIterator();
	if (!pIter) return out;
	for (; !pIter->done(); pIter->next()) {
		AcString s;
		if (pIter->name(s)) out.append(py::cast(s));
	}
	delete pIter;
	return out;
}

py::list PyDbDictionary::values(int mode) {
	py::list out;
	const int total = numEntries();
	for (int i = 0; i < total; ++i) out.append(std::get<1>(entryAt(i, mode)));
	return out;
}

py::list PyDbDictionary::items(int mode) {
	py::list out;
	const int total = numEntries();
	for (int i = 0; i < total; ++i) {
		auto e = entryAt(i, mode);
		out.append(py::make_tuple(py::cast(std::get<0>(e)), std::get<1>(e)));
	}
	return out;
}

std::tuple<AcString, PyDbObject> PyDbDictionary::entryAt(int index, int mode) const {
	if (!impObj()) return std::make_tuple(AcString(), PyDbObject(nullptr));
	const int total = numEntries();
	if (index < 0) index += total;
	if (index < 0 || index >= total) throw py::index_error("Dictionary 索引越界");
	int i = 0;
	AcString found;
	AcDbDictionaryIterator* pIter = impObj()->newIterator();
	if (!pIter) throw py::index_error("Dictionary 无迭代器");
	for (; !pIter->done(); pIter->next(), ++i) {
		if (i != index) continue;
		pIter->name(found);
		break;
	}
	delete pIter;
	if (found.isEmpty()) throw py::index_error("Dictionary 索引越界");
	AcDbObject* p = nullptr;
	if (!PyWarnIfError(impObj()->getAt(found.kwszPtr(), p, static_cast<AcDb::OpenMode>(mode)), L"Dictionary.entryAt"))
		return std::make_tuple(found, PyDbObject(nullptr));
	return std::make_tuple(found, PyDbObject(p));
}
void bind_PyDbDictionary(pym& m) {
	py::class_<PyDbDictionary, PyDbObject>(m, "PyDbDictionary")
		.def(py::init<>())
		.def(py::init<AcDbDictionary*>(), py::arg("object"))
		.def(py::init([]() { return std::make_unique<PyDbDictionary>(new AcDbDictionary()).release(); }), "无参构造（new 默认 AcDbDictionary）")
		.def("add", &PyDbDictionary::add, py::arg("name"), py::arg("obj"))
		.def("getAt", &PyDbDictionary::getAt, py::arg("name"))
		.def("remove", &PyDbDictionary::remove, py::arg("name"))
		.def("numEntries", &PyDbDictionary::numEntries)
		.def("has", &PyDbDictionary::has, py::arg("name"))
		.def("getIdAt", &PyDbDictionary::getIdAt, py::arg("name"), "按名称取条目 ObjectId，失败返回 0")
		.def("nameAt", &PyDbDictionary::nameAt, py::arg("objId"), "由 ObjectId 反查条目名")
		.def("removeById", &PyDbDictionary::removeById, py::arg("objId"), "按 ObjectId 删除条目")
		.def("hasId", &PyDbDictionary::hasId, py::arg("objId"), "按 ObjectId 判断条目是否存在")
		.def("rename", &PyDbDictionary::rename, py::arg("oldName"), py::arg("newName"))
		.def("setName", &PyDbDictionary::rename, py::arg("oldName"), py::arg("newName"), "重命名条目（CHM setName，同 rename）")
		.def("setAt", &PyDbDictionary::setAt, py::arg("key"), py::arg("obj"), "添加/替换条目，返回 (ok, newObjId)")
		.def("mergeStyle", [](const PyDbDictionary& s) { return static_cast<int>(s.mergeStyle()); }, "返回 AcDb::DuplicateRecordCloning 整数")
		.def("setMergeStyle", &PyDbDictionary::setMergeStyle, py::arg("style"), "设置 AcDb::DuplicateRecordCloning")
		.def("isTreatElementsAsHard", &PyDbDictionary::isTreatElementsAsHard)
		.def("setTreatElementsAsHard", &PyDbDictionary::setTreatElementsAsHard, py::arg("doIt"))
		.def("toDict", &PyDbDictionary::toDict)
		// ---- 容器协议：keys/values/items、for 遍历、len、in、索引 ----
		.def("keys", &PyDbDictionary::names, "条目名列表")
		.def("names", &PyDbDictionary::names, "条目名列表")
		.def("values", &PyDbDictionary::values, py::arg("mode") = 1, "条目对象列表")
		.def("items", &PyDbDictionary::items, py::arg("mode") = 1, "(条目名, 对象) 列表")
		.def("entryAt", &PyDbDictionary::entryAt, py::arg("index"), py::arg("mode") = 1, "按位置取 (条目名, 对象)")
		.def("__len__", &PyDbDictionary::numEntries, "len(dict)")
		.def("__contains__", &PyDbDictionary::has, py::arg("name"), "'name' in dict")
		.def("__iter__", [](PyDbDictionary& self) {
			return self.names().attr("__iter__")();
		}, "for name in dict：遍历条目名（取值用 dict[name]）")
		.def("__getitem__", [](PyDbDictionary& self, py::object key) -> py::object {
			if (py::isinstance<py::int_>(key)) {
				return py::cast(std::get<1>(self.entryAt(key.cast<int>(), 1)));
			}
			return self.getAt(key.cast<AcString>());
		}, py::arg("key"), "dict[name] 取条目对象；dict[i] 按位置取第 i 个条目对象（支持负索引）")
		// ---- DictionaryWithDefault 扩展（对象确为默认字典时生效）----
		.def("defaultEntryId", [](PyDbDictionary& self) -> uint64_t {
			auto* p = AcDbDictionaryWithDefault::cast(self.impObj());
			if (!p) return 0ULL;
			AcDbObjectId id = p->defaultId();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		}, "默认条目的 ObjectId（非默认字典返回 0）")
		.def("setDefaultEntry", [](PyDbDictionary& self, uint64_t objId) {
			auto* p = AcDbDictionaryWithDefault::cast(self.impObj());
			if (!p) return false;
			AcDbObjectId id;
			id.setFromOldId(static_cast<Adesk::UIntPtr>(objId));
			return PyWarnIfError(p->setDefaultId(id), L"Dictionary.setDefaultEntry");
		}, py::arg("obj_id"), "设置默认条目 ObjectId")
		PyWrapper_BIND(DbDictionary)
		;
}
