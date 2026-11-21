#pragma once
#include "stdafx.h"

class AcDbFilteredBlockIterator;

// SDK: AcDbFilteredBlockIterator（dbindex.h，抽象纯虚类，无 Py 基类）。
// 构造需要 AcDbFilter 过滤条件（工程未封装 AcDbFilter），只提供指针接管构造。
// 语义：start() 复位；next() 前进并返回下一个 ObjectId（空 id 表示结束）；id() 返回当前 id。
class PyDbFilteredBlockIterator {
public:
    explicit PyDbFilteredBlockIterator(AcDbFilteredBlockIterator* p);
    virtual ~PyDbFilteredBlockIterator();

    PyDbFilteredBlockIterator(const PyDbFilteredBlockIterator&) = delete;
    PyDbFilteredBlockIterator& operator=(const PyDbFilteredBlockIterator&) = delete;

    // 复位迭代（返回是否成功）
    bool start();
    // 当前元素 id（uint64）；失败返回 0
    uint64_t currentId() const;
    // 前进并返回下一个 id（uint64）；结束返回 0
    uint64_t nextId();

    PyDbFilteredBlockIterator& __iter__() { start(); return *this; }
    uint64_t __next__();

    AcDbFilteredBlockIterator* impObj() { return m_pIter; }
    const AcDbFilteredBlockIterator* impObj() const { return m_pIter; }

protected:
    PyDbFilteredBlockIterator() : m_pIter(nullptr) {}
    void reset(AcDbFilteredBlockIterator* p) { m_pIter = p; }

private:
    AcDbFilteredBlockIterator* m_pIter = nullptr;
};

void bind_PyDbFilteredBlockIterator(pym& m);
