#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyUtils.h"
#include "PyDbDictionary.h"

PyWrapper_Cpp(DbDictionary);

bool  PyDbDictionary::add(const AcString& name, PyDbObject& obj) {
	AcDbObjectId retId;
	return PyWarnIfError(impObj()->setAt(name.kwszPtr(), obj.impObj(), retId), L"Dictionary.add");
}

PyDbObject PyDbDictionary::getAt(const AcString& name) {
	AcDbObject* p = nullptr;
	if(!PyWarnIfError(impObj()->getAt(name.kwszPtr(), p, AcDb::kForRead), L"Dictionary.getAt")) return PyDbObject(nullptr);
	return PyDbObject(p);
}

bool  PyDbDictionary::remove(const AcString& name) {
	return PyWarnIfError(impObj()->remove(name.kwszPtr()), L"Dictionary.remove");
}

int PyDbDictionary::numEntries() const {
	return static_cast<int>(impObj()->numEntries());
}

bool PyDbDictionary::has(const AcString& name) const {
	return impObj()->has(name.kwszPtr());
}

uint64_t PyDbDictionary::getIdAt(const AcString& name) const {
	AcDbObjectId id;
	if (!PyWarnIfError(impObj()->getAt(name.kwszPtr(), id), L"Dictionary.getIdAt")) return 0ULL;
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}

AcString PyDbDictionary::nameAt(uint64_t objId) const {
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(objId));
	AcString s;
	if (!PyWarnIfError(impObj()->nameAt(id, s), L"Dictionary.nameAt")) return AcString();
	return s;
}

bool PyDbDictionary::removeById(uint64_t objId) {
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(objId));
	return PyWarnIfError(impObj()->remove(id), L"Dictionary.removeById");
}

bool PyDbDictionary::hasId(uint64_t objId) const {
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(objId));
	return impObj()->has(id);
}

bool PyDbDictionary::rename(const AcString& oldName, const AcString& newName) {
	return impObj()->setName(oldName.kwszPtr(), newName.kwszPtr());
}

std::tuple<bool, uint64_t> PyDbDictionary::setAt(const AcString& key, PyDbObject& obj) {
	AcDbObjectId retId;
	bool ok = PyWarnIfError(impObj()->setAt(key.kwszPtr(), obj.impObj(), retId), L"Dictionary.setAt");
	uint64_t idv = (ok && !retId.isNull()) ? static_cast<uint64_t>(retId.asOldId()) : 0ULL;
	return std::make_tuple(ok, idv);
}

AcDb::DuplicateRecordCloning PyDbDictionary::mergeStyle() const {
	return impObj()->mergeStyle();
}

void PyDbDictionary::setMergeStyle(int style) {
	impObj()->setMergeStyle(static_cast<AcDb::DuplicateRecordCloning>(style));
}

bool PyDbDictionary::isTreatElementsAsHard() const {
	return impObj()->isTreatElementsAsHard();
}

void PyDbDictionary::setTreatElementsAsHard(bool doIt) {
	impObj()->setTreatElementsAsHard(doIt);
}

py::dict PyDbDictionary::toDict() {
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

void bind_PyDbDictionary(pym& m) {
	py::class_<PyDbDictionary, PyDbObject>(m, "PyDbDictionary")
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
