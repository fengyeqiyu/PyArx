#pragma once
#include "stdafx.h"

class PyDbBlockTableRecord;
class AcDbBlockReferenceIdIterator;

// SDK: AcDbBlockReferenceIdIterator（dbsymtb.h，无 Py 基类）。
// 由 BlockTableRecord::newBlockReferenceIdIterator 创建；step() 无参，done() 内联。
// 当前元素返回 BlockReference ObjectId（uint64）。
class PyDbBlockReferenceIdIterator {
public:
    explicit PyDbBlockReferenceIdIterator(PyDbBlockTableRecord& owner);
    explicit PyDbBlockReferenceIdIterator(AcDbBlockReferenceIdIterator* p);
    ~PyDbBlockReferenceIdIterator();

    PyDbBlockReferenceIdIterator(const PyDbBlockReferenceIdIterator&) = delete;
    PyDbBlockReferenceIdIterator& operator=(const PyDbBlockReferenceIdIterator&) = delete;

    bool done() const;
    bool step();
    uint64_t currentId() const;

    PyDbBlockReferenceIdIterator& __iter__() { return *this; }
    uint64_t __next__();

    AcDbBlockReferenceIdIterator* impObj() { return m_pIter; }
    const AcDbBlockReferenceIdIterator* impObj() const { return m_pIter; }

private:
    AcDbBlockReferenceIdIterator* m_pIter = nullptr;
};

void bind_PyDbBlockReferenceIdIterator(pym& m);
