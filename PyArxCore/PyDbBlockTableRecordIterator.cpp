#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbBlockTableRecordIterator.h"
#include "PyDbBlockTableRecord.h"
#include "PyDbEntity.h"

PyDbBlockTableRecordIterator::PyDbBlockTableRecordIterator(PyDbBlockTableRecord& owner, bool atBeginning, bool skipDeleted) {
    if (!owner.impObj()) { m_pIter = nullptr; return; }
    AcDbBlockTableRecordIterator* p = nullptr;
    if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted),
                       L"AcDbBlockTableRecord::newIterator")) {
        m_pIter = nullptr;
        return;
    }
    m_pIter = p;
}

PyDbBlockTableRecordIterator::PyDbBlockTableRecordIterator(AcDbBlockTableRecordIterator* p) : m_pIter(p) {}
PyDbBlockTableRecordIterator::~PyDbBlockTableRecordIterator() { delete m_pIter; }

bool PyDbBlockTableRecordIterator::done() const {
    if (!m_pIter) return true;
    return m_pIter->done();
}

bool PyDbBlockTableRecordIterator::step(bool forward, bool skipDeleted) {
    if (!m_pIter) return false;
    m_pIter->step(forward, skipDeleted);
    return true;
}

uint64_t PyDbBlockTableRecordIterator::currentId() const {
    if (!m_pIter) return 0ULL;
    AcDbObjectId id;
    if (m_pIter->getEntityId(id) != Acad::eOk || id.isNull()) return 0ULL;
    return static_cast<uint64_t>(id.asOldId());
}

PyDbEntity PyDbBlockTableRecordIterator::getEntity(int mode, bool openErasedEntity) const {
    if (!m_pIter) return PyDbEntity(nullptr);
    AcDbEntity* pEntity = nullptr;
    if (m_pIter->getEntity(pEntity, static_cast<AcDb::OpenMode>(mode), openErasedEntity) != Acad::eOk)
        pEntity = nullptr;
    return PyDbEntity(pEntity);
}

uint64_t PyDbBlockTableRecordIterator::__next__() {
    if (!m_pIter || m_pIter->done()) throw py::stop_iteration();
    uint64_t id = currentId();
    m_pIter->step(true, true);
    return id;
}

void bind_PyDbBlockTableRecordIterator(pym& m) {
    py::class_<PyDbBlockTableRecordIterator>(m, "PyDbBlockTableRecordIterator")
        .def(py::init<PyDbBlockTableRecord&, bool, bool>(),
             py::arg("owner"), py::arg("atBeginning") = true, py::arg("skipDeleted") = true,
             "从 PyDbBlockTableRecord 创建实体迭代器")
        .def(py::init<AcDbBlockTableRecordIterator*>(), py::arg("p"), "指针接管构造（析构 delete）")
        .def("done", &PyDbBlockTableRecordIterator::done)
        .def("step", &PyDbBlockTableRecordIterator::step,
             py::arg("forward") = true, py::arg("skipDeleted") = true)
        .def("currentId", &PyDbBlockTableRecordIterator::currentId, "当前实体 ObjectId（uint64）；失败返回 0")
        .def("getEntity", &PyDbBlockTableRecordIterator::getEntity,
             py::arg("mode") = 0, py::arg("openErasedEntity") = false,
             "直接打开当前实体包装；mode 0读/1写/2通知；失败返回空包装")
        .def("__iter__", [](PyDbBlockTableRecordIterator& it) -> PyDbBlockTableRecordIterator& { return it; })
        .def("__next__", &PyDbBlockTableRecordIterator::__next__);
}
