#pragma once
#include "stdafx.h"
#include "PyRxObject.h"

class AcDbObjectIterator;

// AcDbObjectIterator（SDK 头 dbmain.h：class AcDbObjectIterator : public AcRxObject, public AcHeapOperators）
// 实体空间迭代器；由 AcDbObject::newIterator() 创建，本包装接管指针（析构 delete）。
// P2 批次：仅绑 Python 迭代协议（done / step / __iter__ / __next__），不绑其余方法。
class PyDbObjectIterator : public PyRxObject {
public:
	explicit PyDbObjectIterator(AcDbObjectIterator* p) : PyRxObject(p, true, false) {}
	AcDbObjectIterator* impObj() override;
	AcDbObjectIterator* impObj() const override;
};

void bind_PyDbObjectIterator(pym& m);
