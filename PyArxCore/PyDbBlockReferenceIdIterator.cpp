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

void bind_PyDbBlockReferenceIdIterator(pym&) {
    // 公开迭代器已退役：块参照 id 遍历折叠进 PyDbBlockTableRecord.refIds()。
    // PyDbBlockReferenceIdIterator 保留为内部实现。
}
