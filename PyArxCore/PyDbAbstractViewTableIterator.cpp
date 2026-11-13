#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbAbstractViewTableIterator.h"

PyDbAbstractViewTableIterator::PyDbAbstractViewTableIterator(AcDbAbstractViewTableIterator* p)
    : m_pIter(p) {}

PyDbAbstractViewTableIterator::~PyDbAbstractViewTableIterator() {
    delete m_pIter;
}

bool PyDbAbstractViewTableIterator::done() const {
    if (!m_pIter) return true;
    return m_pIter->done();
}

bool PyDbAbstractViewTableIterator::step(bool forward, bool skipDeleted) {
    if (!m_pIter) return false;
    m_pIter->step(forward, skipDeleted);
    return true;
}

uint64_t PyDbAbstractViewTableIterator::currentId() const {
    if (!m_pIter) return 0ULL;
    AcDbObjectId id;
    if (m_pIter->getRecordId(id) != Acad::eOk || id.isNull()) return 0ULL;
    return static_cast<uint64_t>(id.asOldId());
}

uint64_t PyDbAbstractViewTableIterator::__next__() {
    if (!m_pIter || m_pIter->done()) throw py::stop_iteration();
    uint64_t id = currentId();
    m_pIter->step(true, true);
    return id;
}

void bind_PyDbAbstractViewTableIterator(pym& m) {
    py::class_<PyDbAbstractViewTableIterator>(m, "PyDbAbstractViewTableIterator")
        .def(py::init<AcDbAbstractViewTableIterator*>(), py::arg("p"), "指针接管构造（析构 delete）")
        .def("done", &PyDbAbstractViewTableIterator::done, "迭代器是否已到末尾")
        .def("step", &PyDbAbstractViewTableIterator::step,
             py::arg("forward") = true, py::arg("skipDeleted") = true, "前进一格；失败返回 false")
        .def("currentId", &PyDbAbstractViewTableIterator::currentId,
             "当前记录 ObjectId（uint64）；失败返回 0")
        .def("__iter__", [](PyDbAbstractViewTableIterator& it) -> PyDbAbstractViewTableIterator& { return it; })
        .def("__next__", &PyDbAbstractViewTableIterator::__next__);
}
