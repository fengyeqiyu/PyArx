#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbViewportTableIterator.h"
#include "PyDbViewportTable.h"

PyDbViewportTableIterator::PyDbViewportTableIterator(PyDbViewportTable& owner, bool atBeginning, bool skipDeleted) {
    if (!owner.impObj()) { reset(nullptr); return; }
    AcDbViewportTableIterator* p = nullptr;
    if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted),
                       L"AcDbViewportTable::newIterator")) {
        reset(nullptr);
        return;
    }
    reset(static_cast<AcDbAbstractViewTableIterator*>(p));
}

PyDbViewportTableIterator::PyDbViewportTableIterator(AcDbViewportTableIterator* p)
    : PyDbAbstractViewTableIterator(static_cast<AcDbAbstractViewTableIterator*>(p)) {}

AcDbViewportTableIterator* PyDbViewportTableIterator::impObj() {
    return static_cast<AcDbViewportTableIterator*>(PyDbAbstractViewTableIterator::impObj());
}
const AcDbViewportTableIterator* PyDbViewportTableIterator::impObj() const {
    return static_cast<const AcDbViewportTableIterator*>(PyDbAbstractViewTableIterator::impObj());
}

void bind_PyDbViewportTableIterator(pym&) {
    // 公开迭代器已退役；视口表遍历由 PyDbSymbolTable 容器协议覆盖。
}
