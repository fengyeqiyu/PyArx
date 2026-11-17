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

void bind_PyDbUCSTableIterator(pym&) {
    // 公开迭代器已退役；UCS 表遍历由 PyDbSymbolTable 容器协议覆盖。
}
