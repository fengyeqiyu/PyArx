#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

class PyDbBlockTableRecord;
class AcDbBlockTableRecordIterator;

// SDK: AcDbBlockTableRecordIterator（dbsymtb.h，无 Py 基类）。
// 遍历 BlockTableRecord 内的实体；当前元素返回实体 ObjectId（uint64）。
class PyDbBlockTableRecordIterator {
public:
    PyDbBlockTableRecordIterator(PyDbBlockTableRecord& owner, bool atBeginning = true, bool skipDeleted = true);
    explicit PyDbBlockTableRecordIterator(AcDbBlockTableRecordIterator* p);
    ~PyDbBlockTableRecordIterator();

    PyDbBlockTableRecordIterator(const PyDbBlockTableRecordIterator&) = delete;
    PyDbBlockTableRecordIterator& operator=(const PyDbBlockTableRecordIterator&) = delete;

    bool done() const;
    bool step(bool forward = true, bool skipDeleted = true);
    uint64_t currentId() const;

    // 直接打开当前实体包装；mode: 0=kForRead, 1=kForWrite, 2=kForNotify；失败返回空包装
    PyDbEntity getEntity(int mode = 0, bool openErasedEntity = false) const;

    PyDbBlockTableRecordIterator& __iter__() { return *this; }
    uint64_t __next__();

    AcDbBlockTableRecordIterator* impObj() { return m_pIter; }
    const AcDbBlockTableRecordIterator* impObj() const { return m_pIter; }

private:
    AcDbBlockTableRecordIterator* m_pIter = nullptr;
};

void bind_PyDbBlockTableRecordIterator(pym& m);
