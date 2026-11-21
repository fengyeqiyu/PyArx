#pragma once
#include "stdafx.h"

class AcDbCompositeFilteredBlockIterator;

// SDK: AcDbCompositeFilteredBlockIterator（dbindex.h，抽象纯虚类，无 Py 基类）。
// init() 需要 AcDbFilter** 数组 + BlockTableRecord（工程未封装 AcDbFilter），
// 只提供指针接管构造；调用方需自行完成 init() 后再交给本包装。
class PyDbCompositeFilteredBlockIterator {
public:
    explicit PyDbCompositeFilteredBlockIterator(AcDbCompositeFilteredBlockIterator* p);
    ~PyDbCompositeFilteredBlockIterator();

    PyDbCompositeFilteredBlockIterator(const PyDbCompositeFilteredBlockIterator&) = delete;
    PyDbCompositeFilteredBlockIterator& operator=(const PyDbCompositeFilteredBlockIterator&) = delete;

    bool start();
    uint64_t currentId() const;
    uint64_t nextId();

    PyDbCompositeFilteredBlockIterator& __iter__() { start(); return *this; }
    uint64_t __next__();

    AcDbCompositeFilteredBlockIterator* impObj() { return m_pIter; }
    const AcDbCompositeFilteredBlockIterator* impObj() const { return m_pIter; }

private:
    AcDbCompositeFilteredBlockIterator* m_pIter = nullptr;
};

void bind_PyDbCompositeFilteredBlockIterator(pym& m);
