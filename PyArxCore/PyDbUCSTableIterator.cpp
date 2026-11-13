#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbUCSTableIterator.h"
#include "PyDbUCSTable.h"

PyDbUCSTableIterator::PyDbUCSTableIterator(PyDbUCSTable& owner, bool atBeginning, bool skipDeleted) {
    if (!owner.impObj()) { m_pIter = nullptr; return; }
    AcDbUCSTableIterator* p = nullptr;
    if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted),
                       L"AcDbUCSTable::newIterator")) {
        m_pIter = nullptr;
        return;
    }
    m_pIter = p;
}

PyDbUCSTableIterator::PyDbUCSTableIterator(AcDbUCSTableIterator* p) : m_pIter(p) {}
PyDbUCSTableIterator::~PyDbUCSTableIterator() { delete m_pIter; }

bool PyDbUCSTableIterator::done() const {
    if (!m_pIter) return true;
    return m_pIter->done();
}

bool PyDbUCSTableIterator::step(bool forward, bool skipDeleted) {
    if (!m_pIter) return false;
    m_pIter->step(forward, skipDeleted);
    return true;
}

uint64_t PyDbUCSTableIterator::currentId() const {
    if (!m_pIter) return 0ULL;
    AcDbObjectId id;
    if (m_pIter->getRecordId(id) != Acad::eOk || id.isNull()) return 0ULL;
    return static_cast<uint64_t>(id.asOldId());
}

uint64_t PyDbUCSTableIterator::__next__() {
    if (!m_pIter || m_pIter->done()) throw py::stop_iteration();
    uint64_t id = currentId();
    m_pIter->step(true, true);
    return id;
}

void bind_PyDbUCSTableIterator(pym& m) {
    py::class_<PyDbUCSTableIterator>(m, "PyDbUCSTableIterator")
        .def(py::init<PyDbUCSTable&, bool, bool>(),
             py::arg("owner"), py::arg("atBeginning") = true, py::arg("skipDeleted") = true,
             "从 PyDbUCSTable 创建迭代器")
        .def(py::init<AcDbUCSTableIterator*>(), py::arg("p"), "指针接管构造（析构 delete）")
        .def("done", &PyDbUCSTableIterator::done)
        .def("step", &PyDbUCSTableIterator::step,
             py::arg("forward") = true, py::arg("skipDeleted") = true)
        .def("currentId", &PyDbUCSTableIterator::currentId, "当前记录 ObjectId（uint64）；失败返回 0")
        .def("__iter__", [](PyDbUCSTableIterator& it) -> PyDbUCSTableIterator& { return it; })
        .def("__next__", &PyDbUCSTableIterator::__next__);
}
