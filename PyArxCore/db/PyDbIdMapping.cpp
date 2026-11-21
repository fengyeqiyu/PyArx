#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbIdMapping.h"
#include "PyDbDatabase.h"

namespace {
	AcDbObjectId fromOld(uint64_t v) {
		AcDbObjectId id;
		id.setFromOldId(static_cast<Adesk::UIntPtr>(v));
		return id;
	}
	uint64_t toOld(const AcDbObjectId& id) {
		return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
	}
}

PyDbIdMapping::~PyDbIdMapping() {
	if (m_own && m_ptr) delete m_ptr;
	m_ptr = nullptr;
}

std::unique_ptr<PyDbIdMapping> PyDbIdMapping::create() {
	return std::unique_ptr<PyDbIdMapping>(new PyDbIdMapping(new AcDbIdMapping(), true));
}

bool PyDbIdMapping::assign(uint64_t keyId, uint64_t valueId, bool isCloned,
	bool isPrimary, bool isOwnerXlated) {
	if (!m_ptr) return false;
	AcDbIdPair pair(fromOld(keyId), fromOld(valueId), isCloned, isPrimary, isOwnerXlated);
	return PyWarnIfError(m_ptr->assign(pair), L"IdMapping.assign");
}

uint64_t PyDbIdMapping::compute(uint64_t keyId) const {
	if (!m_ptr) return 0ULL;
	AcDbIdPair pair(fromOld(keyId), AcDbObjectId::kNull, false);
	if (!m_ptr->compute(pair)) return 0ULL;
	return toOld(pair.value());
}

bool PyDbIdMapping::change(uint64_t keyId, uint64_t valueId, bool isCloned,
	bool isPrimary, bool isOwnerXlated) {
	if (!m_ptr) return false;
	AcDbIdPair pair(fromOld(keyId), fromOld(valueId), isCloned, isPrimary, isOwnerXlated);
	return m_ptr->change(pair);
}

bool PyDbIdMapping::del(uint64_t keyId) {
	return m_ptr ? m_ptr->del(fromOld(keyId)) : false;
}

bool PyDbIdMapping::setDestDb(PyDbDatabase& db) {
	if (!m_ptr) return false;
	return PyWarnIfError(m_ptr->setDestDb(db.impObj()), L"IdMapping.setDestDb");
}

bool PyDbIdMapping::copyFrom(const PyDbIdMapping& other) {
	if (!m_ptr || !other.m_ptr) return false;
	return PyWarnIfError(m_ptr->copyFrom(other.m_ptr), L"IdMapping.copyFrom");
}

PyDbDatabase PyDbIdMapping::destDb() const {
	if (!m_ptr) return PyDbDatabase(nullptr);
	AcDbDatabase* pDb = nullptr;
	if (!PyWarnIfError(m_ptr->destDb(pDb), L"IdMapping.destDb")) return PyDbDatabase(nullptr);
	return PyDbDatabase(pDb, false);
}

PyDbDatabase PyDbIdMapping::origDb() const {
	if (!m_ptr) return PyDbDatabase(nullptr);
	AcDbDatabase* pDb = nullptr;
	if (!PyWarnIfError(m_ptr->origDb(pDb), L"IdMapping.origDb")) return PyDbDatabase(nullptr);
	return PyDbDatabase(pDb, false);
}

int PyDbIdMapping::deepCloneContext() const {
	return m_ptr ? static_cast<int>(m_ptr->deepCloneContext()) : 0;
}

int PyDbIdMapping::duplicateRecordCloning() const {
	return m_ptr ? static_cast<int>(m_ptr->duplicateRecordCloning()) : 0;
}

AcString PyDbIdMapping::toString() const {
	return AcString(std::format("PyDbIdMapping[{}]", m_ptr ? "valid" : "null").c_str(), AcString::Utf8);
}

void bind_PyDbIdMapping(pym& m) {
	py::class_<PyDbIdMapping>(m, "PyDbIdMapping")
		.def(py::init<>())
		.def_static("create", &PyDbIdMapping::create)
		.def("isNull", &PyDbIdMapping::isNull)
		.def("assign", &PyDbIdMapping::assign,
			py::arg("keyId"), py::arg("valueId"), py::arg("isCloned"),
			py::arg("isPrimary") = false, py::arg("isOwnerXlated") = true)
		.def("compute", &PyDbIdMapping::compute, py::arg("keyId"))
		.def("change", &PyDbIdMapping::change,
			py::arg("keyId"), py::arg("valueId"), py::arg("isCloned"),
			py::arg("isPrimary") = false, py::arg("isOwnerXlated") = true)
		.def("del", &PyDbIdMapping::del, py::arg("keyId"))
		.def("setDestDb", &PyDbIdMapping::setDestDb, py::arg("db"))
		.def("copyFrom", &PyDbIdMapping::copyFrom, py::arg("other"),
			"用另一个 IdMapping 的内容覆盖当前对象。")
		.def("destDb", &PyDbIdMapping::destDb, "返回目标数据库（失败返回空 PyDbDatabase）。")
		.def("origDb", &PyDbIdMapping::origDb, "返回原始数据库（失败返回空 PyDbDatabase）。")
		.def("deepCloneContext", &PyDbIdMapping::deepCloneContext)
		.def("duplicateRecordCloning", &PyDbIdMapping::duplicateRecordCloning)
		.def("__repr__", &PyDbIdMapping::toString);
}
