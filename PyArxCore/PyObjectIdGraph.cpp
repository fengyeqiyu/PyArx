#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyObjectIdGraph.h"

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

PyObjectIdGraph::PyObjectIdGraph(AcDbObjectIdGraph* p, bool own) : m_ptr(p), m_own(own) {}

PyObjectIdGraph::~PyObjectIdGraph() {
	if (m_own && m_ptr) delete m_ptr;
	m_ptr = nullptr;
}

std::unique_ptr<PyObjectIdGraph> PyObjectIdGraph::create() {
	return std::make_unique<PyObjectIdGraph>(new AcDbObjectIdGraph(), true);
}

int PyObjectIdGraph::numNodes() const {
	return m_ptr ? m_ptr->numNodes() : 0;
}

bool PyObjectIdGraph::isEmpty() const {
	return !m_ptr || m_ptr->isEmpty();
}

bool PyObjectIdGraph::addNode(uint64_t objectId) {
	if (!m_ptr) return false;
	auto* node = new AcDbObjectIdGraphNode(fromOld(objectId));
	if (!PyWarnIfError(m_ptr->addNode(node), L"ObjectIdGraph.addNode")) {
		delete node;
		return false;
	}
	return true;
}

bool PyObjectIdGraph::delNode(uint64_t objectId) {
	if (!m_ptr) return false;
	AcDbObjectIdGraphNode* node = m_ptr->findNode(fromOld(objectId));
	if (!node) return false;
	return PyWarnIfError(m_ptr->delNode(node), L"ObjectIdGraph.delNode");
}

uint64_t PyObjectIdGraph::findNode(uint64_t objectId) const {
	if (!m_ptr) return 0ULL;
	AcDbObjectIdGraphNode* node = m_ptr->findNode(fromOld(objectId));
	return node ? toOld(node->id()) : 0ULL;
}

uint64_t PyObjectIdGraph::idNode(int index) const {
	if (!m_ptr || index < 0 || index >= m_ptr->numNodes()) return 0ULL;
	AcDbObjectIdGraphNode* node = m_ptr->idNode(index);
	return node ? toOld(node->id()) : 0ULL;
}

AcString PyObjectIdGraph::toString() const {
	return AcString(std::format("PyObjectIdGraph[nodes={}]", numNodes()).c_str(), AcString::Utf8);
}

void bind_PyObjectIdGraph(pym& m) {
	py::class_<PyObjectIdGraph>(m, "PyObjectIdGraph")
		.def(py::init<>())
		.def_static("create", &PyObjectIdGraph::create)
		.def("isNull", &PyObjectIdGraph::isNull)
		.def("numNodes", &PyObjectIdGraph::numNodes)
		.def("isEmpty", &PyObjectIdGraph::isEmpty)
		.def("addNode", &PyObjectIdGraph::addNode, py::arg("objectId"))
		.def("delNode", &PyObjectIdGraph::delNode, py::arg("objectId"))
		.def("findNode", &PyObjectIdGraph::findNode, py::arg("objectId"))
		.def("idNode", &PyObjectIdGraph::idNode, py::arg("index"))
		.def("__repr__", &PyObjectIdGraph::toString);
}
