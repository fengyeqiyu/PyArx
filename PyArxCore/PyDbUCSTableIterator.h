#pragma once
#include "stdafx.h"

class PyDbUCSTable;
class AcDbUCSTableIterator;

// SDK: AcDbUCSTableIterator : AcDbSymbolTableIterator（dbsymtb.h）。
// 工程无 PyDbSymbolTableIterator，本类独立绑定（不使用 PyWrapper_* 宏）。
class PyDbUCSTableIterator {
public:
    PyDbUCSTableIterator(PyDbUCSTable& owner, bool atBeginning = true, bool skipDeleted = true);
    explicit PyDbUCSTableIterator(AcDbUCSTableIterator* p);
    ~PyDbUCSTableIterator();

    PyDbUCSTableIterator(const PyDbUCSTableIterator&) = delete;
    PyDbUCSTableIterator& operator=(const PyDbUCSTableIterator&) = delete;

    bool done() const;
    bool step(bool forward = true, bool skipDeleted = true);
    uint64_t currentId() const;

    PyDbUCSTableIterator& __iter__() { return *this; }
    uint64_t __next__();

    AcDbUCSTableIterator* impObj() { return m_pIter; }
    const AcDbUCSTableIterator* impObj() const { return m_pIter; }

private:
    AcDbUCSTableIterator* m_pIter = nullptr;
};

void bind_PyDbUCSTableIterator(pym& m);
