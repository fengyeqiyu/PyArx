#pragma once
#include "stdafx.h"

class AcDbAbstractViewTableIterator;

// 抽象基类包装（SDK: AcDbAbstractViewTableIterator : AcDbSymbolTableIterator，dbsymtb.h）。
// 本身无公开构造路径（SDK 构造为 protected），只提供指针接管构造。
// View/Viewport 迭代器继承本类，迭代协议方法在本类实现，派生类自动透出。
class PyDbAbstractViewTableIterator {
public:
    explicit PyDbAbstractViewTableIterator(AcDbAbstractViewTableIterator* p);
    virtual ~PyDbAbstractViewTableIterator();

    PyDbAbstractViewTableIterator(const PyDbAbstractViewTableIterator&) = delete;
    PyDbAbstractViewTableIterator& operator=(const PyDbAbstractViewTableIterator&) = delete;

    bool done() const;
    bool step(bool forward = true, bool skipDeleted = true);
    uint64_t currentId() const;

    PyDbAbstractViewTableIterator& __iter__() { return *this; }
    uint64_t __next__();

    AcDbAbstractViewTableIterator* impObj() { return m_pIter; }
    const AcDbAbstractViewTableIterator* impObj() const { return m_pIter; }

protected:
    // 供派生类构造时接管指针（派生类构造失败时置 nullptr）
    PyDbAbstractViewTableIterator() : m_pIter(nullptr) {}
    void reset(AcDbAbstractViewTableIterator* p) { m_pIter = p; }

private:
    AcDbAbstractViewTableIterator* m_pIter = nullptr;
};

void bind_PyDbAbstractViewTableIterator(pym& m);
