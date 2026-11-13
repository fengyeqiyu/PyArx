#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbBlockReferenceIdIterator.h"
#include "PyDbBlockTableRecord.h"

PyDbBlockReferenceIdIterator::PyDbBlockReferenceIdIterator(PyDbBlockTableRecord& owner) {
    if (!owner.impObj()) { m_pIter = nullptr; return; }
    AcDbBlockReferenceIdIterator* p = nullptr;
    if (!PyWarnIfError(owner.impObj()->newBlockReferenceIdIterator(p),
                       L"AcDbBlockTableRecord::newBlockReferenceIdIterator")) {
        m_pIter = nullptr;
        return;
    }
    m_pIter = p;
}

PyDbBlockReferenceIdIterator::PyDbBlockReferenceIdIterator(AcDbBlockReferenceIdIterator* p) : m_pIter(p) {}
PyDbBlockReferenceIdIterator::~PyDbBlockReferenceIdIterator() { delete m_pIter; }

bool PyDbBlockReferenceIdIterator::done() const {
    if (!m_pIter) return true;
    return m_pIter->done();
}

bool PyDbBlockReferenceIdIterator::step() {
    if (!m_pIter) return false;
    m_pIter->step();
    return true;
}

uint64_t PyDbBlockReferenceIdIterator::currentId() const {
    if (!m_pIter) return 0ULL;
    AcDbObjectId id;
    if (m_pIter->getBlockReferenceId(id) != Acad::eOk || id.isNull()) return 0ULL;
    return static_cast<uint64_t>(id.asOldId());
}

uint64_t PyDbBlockReferenceIdIterator::__next__() {
    if (!m_pIter || m_pIter->done()) throw py::stop_iteration();
    uint64_t id = currentId();
    m_pIter->step();
    return id;
}

void bind_PyDbBlockReferenceIdIterator(pym& m) {
    py::class_<PyDbBlockReferenceIdIterator>(m, "PyDbBlockReferenceIdIterator")
        .def(py::init<PyDbBlockTableRecord&>(),
             py::arg("owner"), "从 PyDbBlockTableRecord 创建 BlockReferenceId 迭代器")
        .def(py::init<AcDbBlockReferenceIdIterator*>(), py::arg("p"), "指针接管构造（析构 delete）")
        .def("done", &PyDbBlockReferenceIdIterator::done)
        .def("step", &PyDbBlockReferenceIdIterator::step)
        .def("currentId", &PyDbBlockReferenceIdIterator::currentId, "当前 BlockReference ObjectId（uint64）；失败返回 0")
        .def("__iter__", [](PyDbBlockReferenceIdIterator& it) -> PyDbBlockReferenceIdIterator& {
            if (it.impObj()) it.impObj()->start();
            return it;
        })
        .def("__next__", &PyDbBlockReferenceIdIterator::__next__);
}
