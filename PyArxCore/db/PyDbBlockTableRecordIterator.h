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
    // 默认 1=写：图元操作默认写模式，纯查询显式传 0
    PyDbEntity getEntity(int mode = 1, bool openErasedEntity = false) const;

    PyDbBlockTableRecordIterator& __iter__() { return *this; }
    uint64_t __next__();

    AcDbBlockTableRecordIterator* impObj() { return m_pIter; }
    const AcDbBlockTableRecordIterator* impObj() const { return m_pIter; }

private:
    AcDbBlockTableRecordIterator* m_pIter = nullptr;
};

void bind_PyDbBlockTableRecordIterator(pym& m);

// 块表记录内实体的「直接遍历」迭代器：`for ent in btr` 用，直接产出 PyDbEntity。
// 与 PyDbBlockTableRecordIterator 的分工：后者只吐 ObjectId，本类直接吐实体包装。
class PyDbBlockTableRecordEntityIter {
public:
	PyDbBlockTableRecordEntityIter(PyDbBlockTableRecord& owner, int mode = 1, bool skipDeleted = true);
	~PyDbBlockTableRecordEntityIter();

	PyDbBlockTableRecordEntityIter(const PyDbBlockTableRecordEntityIter&) = delete;
	PyDbBlockTableRecordEntityIter& operator=(const PyDbBlockTableRecordEntityIter&) = delete;

	PyDbBlockTableRecordEntityIter& __iter__() { return *this; }
	py::object __next__();   // 按 AcRxClass 继承链还原真实类型（表格/标注等不再退化成 PyDbEntity）

private:
	AcDbBlockTableRecordIterator* m_pIter = nullptr;
	AcDbEntity* m_pEnt = nullptr;
	int m_mode = 1;
};
