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

void bind_PyDbAbstractViewTableIterator(pym&) {
    // 公开迭代器已退役；视图/视口表遍历由 PyDbSymbolTable 容器协议覆盖。
}
