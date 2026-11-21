#include "stdafx.h"
#include "PyDbFullSubentPath.h"
#include "PyDbTypeCaster.h"
#include "PyRxClass.h"

const PyDbSubentId PyDbSubentId::kNull{ AcDbSubentId::kNull };
const AcDbFullSubentPath PyDbFullSubentPath::kNull{};

// PyDbSubentId ʵ��
PyDbSubentId::PyDbSubentId(AcDbSubentId* pSubentId, bool isOwner)
	: m_ptr(pSubentId), m_isOwner(isOwner)
{
}

PyDbSubentId::PyDbSubentId()
	: PyDbSubentId(new AcDbSubentId(), true)
{
}

PyDbSubentId::PyDbSubentId(const AcDbSubentId& src)
	: m_ptr(new AcDbSubentId(src)), m_isOwner(true)
{
}

PyDbSubentId::PyDbSubentId(AcDbSubentId& subIdRef)
	: m_ptr(&subIdRef), m_isOwner(false)
{
}

PyDbSubentId::PyDbSubentId(const PyDbSubentId& other)
	: m_ptr(new AcDbSubentId(*other.m_ptr)), m_isOwner(true)
{
}

PyDbSubentId::PyDbSubentId(AcDb::SubentType type, Adesk::GsMarker index)
	: m_ptr(new AcDbSubentId(type, index)), m_isOwner(true)
{
}

PyDbSubentId::PyDbSubentId(PyRxClass& pTypeClass, Adesk::GsMarker index)
	: m_ptr(new AcDbSubentId(pTypeClass.impObj(), index)), m_isOwner(true)
{
}

PyDbSubentId::~PyDbSubentId()
{
	if (m_isOwner && m_ptr != nullptr) {
		delete m_ptr;
		m_ptr = nullptr;
	}
}

PyDbSubentId& PyDbSubentId::operator=(const PyDbSubentId& other)
{
	if (this != &other) {
		if (m_isOwner && m_ptr != nullptr) {
			delete m_ptr;
		}
		m_ptr = new AcDbSubentId(*other.m_ptr);
		m_isOwner = true;
	}
	return *this;
}

PyDbSubentId::operator AcDbSubentId() const
{
	if (m_ptr == nullptr)
		return AcDbSubentId::kNull;
	return *m_ptr;
}

bool PyDbSubentId::operator==(const PyDbSubentId& other) const
{
	return *m_ptr == *other.m_ptr;
}

bool PyDbSubentId::operator!=(const PyDbSubentId& other) const
{
	return !(*this == other);
}





AcDb::SubentType PyDbSubentId::type() const {
	if (m_ptr == nullptr)
		return AcDb::kNullSubentType;
	return m_ptr->type();
}
bool PyDbSubentId::setType(AcDb::SubentType subtype) {
	if (m_ptr == nullptr)
		return false;
	m_ptr->setType(subtype);
	return true;
}
PyRxClass PyDbSubentId::typeClass() const {
	if (m_ptr == nullptr)
		return PyRxClass();
	return m_ptr->typeClass();
}
bool PyDbSubentId::setTypeClass(PyRxClass& rxclass) {
	if (m_ptr == nullptr || rxclass.impObj() == nullptr)
		return false;
	m_ptr->setTypeClass(rxclass.impObj());
	return true;
}
Adesk::GsMarker PyDbSubentId::index() const {
	if (m_ptr == nullptr)
		return 0;
	return m_ptr->index();
}
bool PyDbSubentId::setIndex(Adesk::GsMarker index) {
	if (m_ptr == nullptr)
		return false;
	m_ptr->setIndex(index);
	return true;
}








// PyDbFullSubentPath ʵ��
PyDbFullSubentPath::PyDbFullSubentPath(const AcDbFullSubentPath& fsp)
	: m_fsp(fsp)
{
}

PyDbFullSubentPath::PyDbFullSubentPath()
	: m_fsp()
{
}

PyDbFullSubentPath::PyDbFullSubentPath(AcDb::SubentType type, Adesk::GsMarker index)
	: m_fsp(type, index)
{
}

PyDbFullSubentPath::PyDbFullSubentPath(AcDbObjectId entId, AcDb::SubentType type, Adesk::GsMarker index)
	: m_fsp(entId, type, index)
{
}

PyDbFullSubentPath::PyDbFullSubentPath(AcDbObjectId entId, AcDbSubentId subId)
	: m_fsp(entId, subId)
{
}

PyDbFullSubentPath::PyDbFullSubentPath(const AcDbObjectIdArray& objectIds, AcDbSubentId subId)
	: m_fsp(objectIds, subId)
{
}

PyDbFullSubentPath::PyDbFullSubentPath(const PyDbFullSubentPath& other)
	: m_fsp(other.m_fsp)
{
}

PyDbFullSubentPath& PyDbFullSubentPath::operator=(const PyDbFullSubentPath& other)
{
	if (this != &other) {
		m_fsp = other.m_fsp;
	}
	return *this;
}

PyDbFullSubentPath::operator AcDbFullSubentPath() const
{
	return m_fsp;
}

bool PyDbFullSubentPath::operator==(const PyDbFullSubentPath& other) const
{
	return m_fsp == other.m_fsp;
}

bool PyDbFullSubentPath::operator!=(const PyDbFullSubentPath& other) const
{
	return !(*this == other);
}

bool PyDbFullSubentPath::setObjectIds(const std::vector<AcDbObjectId>& objectIds)
{
	m_fsp.objectIds().removeAll();
	for (const auto& objid : objectIds) {
		m_fsp.objectIds().append(objid);
	}
	return true;
}

std::vector<AcDbObjectId> PyDbFullSubentPath::objectIds() const
{
	std::vector<AcDbObjectId> result;
	result.reserve(m_fsp.objectIds().length());
	for (const auto& objid : m_fsp.objectIds()) {
		result.emplace_back(objid);
	}
	return result;
}

bool PyDbFullSubentPath::setSubentId(const PyDbSubentId& subentId)
{
	m_fsp.setSubentId(subentId);
	return true;
}

PyDbSubentId PyDbFullSubentPath::subentId() const
{
	return PyDbSubentId(m_fsp.subentId());
}

PyDbSubentId PyDbFullSubentPath::subentId()
{
	return PyDbSubentId(m_fsp.subentId());
}

void bind_PyDbSubentId(pym& m) {
	py::class_<PyDbSubentId>(m, "SubentId")
		.def(py::init<>())
		.def(py::init<AcDb::SubentType, Adesk::GsMarker>(), py::arg("type"), py::arg("index"))
		.def_property("type", &PyDbSubentId::type, [](PyDbSubentId& s, AcDb::SubentType t) { s.setType(t); })
		.def_property("index", &PyDbSubentId::index, [](PyDbSubentId& s, Adesk::GsMarker i) { s.setIndex(i); })
		.def("setType", &PyDbSubentId::setType, py::arg("type"), "设置子实体类型；成功返回 true")
		.def("setIndex", &PyDbSubentId::setIndex, py::arg("index"), "设置子实体标记；成功返回 true")
		.def("typeClass", &PyDbSubentId::typeClass)
		.def("setTypeClass", &PyDbSubentId::setTypeClass, py::arg("pClass"))
		.def("__eq__", &PyDbSubentId::operator==)
		.def("__ne__", &PyDbSubentId::operator!=)
		.def_readonly_static("kNull", &PyDbSubentId::kNull)
		;
}

void bind_PyDbFullSubentPath(pym& m) {
	py::class_<PyDbFullSubentPath>(m, "FullSubentPath")
		.def(py::init<>())
		.def(py::init<AcDb::SubentType, Adesk::GsMarker>(), py::arg("type"), py::arg("index"))
		.def(py::init([](const AcDbObjectId& entId, AcDb::SubentType type, Adesk::GsMarker index) {
			return new PyDbFullSubentPath(entId, type, index);
		}), py::arg("entId"), py::arg("type"), py::arg("index"))
		.def(py::init([](const AcDbObjectId& entId, const PyDbSubentId& subId) {
			return new PyDbFullSubentPath(entId, static_cast<AcDbSubentId>(subId));
		}), py::arg("entId"), py::arg("subId"))
		.def("objectIds", &PyDbFullSubentPath::objectIds)
		.def("setObjectIds", &PyDbFullSubentPath::setObjectIds, py::arg("objectIds"))
		.def("subentId", static_cast<PyDbSubentId (PyDbFullSubentPath::*)() const>(&PyDbFullSubentPath::subentId))
		.def("setSubentId", &PyDbFullSubentPath::setSubentId, py::arg("subentId"))
		.def("__eq__", &PyDbFullSubentPath::operator==)
		.def("__ne__", &PyDbFullSubentPath::operator!=)
		;
}