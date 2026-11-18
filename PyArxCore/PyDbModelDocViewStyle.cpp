#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbModelDocViewStyle.h"
#include "PyDbDatabase.h"

PyWrapper_Cpp(DbModelDocViewStyle);

AcString PyDbModelDocViewStyle::defaultViewName(int index) const {
	if (!impObj()) return AcString();
	return impObj()->defaultViewName(index);
}

AcString PyDbModelDocViewStyle::getDescription() const {
	AcString s;
	if (!impObj()) return s;
	impObj()->getDescription(s);
	return s;
}

AcString PyDbModelDocViewStyle::getName() const {
	AcString s;
	if (!impObj()) return s;
	if (!PyWarnIfError(impObj()->getName(s), L"AcDbModelDocViewStyle::getName")) return AcString();
	return s;
}

bool PyDbModelDocViewStyle::isModifiedForRecompute() const {
	if (!impObj()) return false;
	return impObj()->isModifiedForRecompute();
}

bool PyDbModelDocViewStyle::isViewNameValid(const AcString& name) const {
	if (!impObj()) return false;
	return impObj()->isViewNameValid(name.kwszPtr());
}

std::tuple<bool, uint64_t> PyDbModelDocViewStyle::postViewStyleToDb(PyDbDatabase& db, const AcString& styleName) {
	if (!impObj() || !db.impObj()) return std::make_tuple(false, 0ULL);
	AcDbObjectId id;
	Acad::ErrorStatus es = impObj()->postViewStyleToDb(db.impObj(), styleName.kwszPtr(), id);
	if (!PyWarnIfError(es, L"AcDbModelDocViewStyle::postViewStyleToDb")) return std::make_tuple(false, 0ULL);
	return std::make_tuple(true, id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()));
}

bool PyDbModelDocViewStyle::setDatabaseDefaults(PyDbDatabase& db) {
	if (!impObj() || !db.impObj()) return false;
	impObj()->setDatabaseDefaults(db.impObj());
	return true;
}

bool PyDbModelDocViewStyle::setDescription(const AcString& desc) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setDescription(desc.kwszPtr()), L"AcDbModelDocViewStyle::setDescription");
}

bool PyDbModelDocViewStyle::setName(const AcString& name) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setName(name.kwszPtr()), L"AcDbModelDocViewStyle::setName");
}

void bind_PyDbModelDocViewStyle(pym& m) {
	py::class_<PyDbModelDocViewStyle, PyDbObject>(m, "PyDbModelDocViewStyle")
		.def(py::init<AcDbModelDocViewStyle*>(), py::arg("p"))
		PyWrapper_BIND(DbModelDocViewStyle)
		.def("defaultViewName", &PyDbModelDocViewStyle::defaultViewName, py::arg("index"))
		.def("getDescription", &PyDbModelDocViewStyle::getDescription)
		.def("getName", &PyDbModelDocViewStyle::getName)
		.def("isModifiedForRecompute", &PyDbModelDocViewStyle::isModifiedForRecompute)
		.def("isViewNameValid", &PyDbModelDocViewStyle::isViewNameValid, py::arg("name"))
		.def("postViewStyleToDb", &PyDbModelDocViewStyle::postViewStyleToDb, py::arg("db"), py::arg("styleName"), "返回 (ok, outObjectId)")
		.def("setDatabaseDefaults", &PyDbModelDocViewStyle::setDatabaseDefaults, py::arg("db"))
		.def("setDescription", &PyDbModelDocViewStyle::setDescription, py::arg("desc"))
		.def("setName", &PyDbModelDocViewStyle::setName, py::arg("name"))
		;
}
