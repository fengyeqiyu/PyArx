#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDictionaryWithDefault.h"

PyWrapper_Cpp(DbDictionaryWithDefault);

// 自身方法：defaultId / setDefaultId / getObjectBirthVersion。
// getObjectBirthVersion 为 AcDbObject 虚函数 override，PyDbObject 已绑，跳过。
void bind_PyDbDictionaryWithDefault(pym& m) {
	py::class_<PyDbDictionaryWithDefault, PyDbDictionary>(m, "PyDbDictionaryWithDefault")
		.def(py::init<AcDbDictionaryWithDefault*>(), py::arg("p"))
		.def(py::init([]() { return std::make_unique<PyDbDictionaryWithDefault>(new AcDbDictionaryWithDefault()).release(); }), "无参构造（new 默认 AcDbDictionaryWithDefault）")
		PyWrapper_BIND(DbDictionaryWithDefault)
		// ---- defaultId() -> uint64 ObjectId ----
		.def("defaultId", [](const PyDbDictionaryWithDefault& s) -> uint64_t {
			AcDbObjectId id = s.impObj()->defaultId();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		}, "默认条目的 ObjectId（0=空）")
		// ---- setDefaultId(objId) -> bool ----
		.def("setDefaultId", [](PyDbDictionaryWithDefault& s, uint64_t idv) -> bool {
			AcDbObjectId id;
			id.setFromOldId(static_cast<Adesk::UIntPtr>(idv));
			return PyWarnIfError(s.impObj()->setDefaultId(id), L"AcDbDictionaryWithDefault::setDefaultId");
		}, py::arg("obj_id"), "设置默认条目 ObjectId")
		;
}
