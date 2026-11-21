#pragma once
#include "stdafx.h"

// SDK: dbindex.h — class AcDbIndexUpdateDataIterator（无基类，独立类）
// ctor 需要 const AcDbIndexUpdateData*，源类在③档未封装，暂无公开创建路径。
// 当前元素 currentData() 中的 AcDbObjectId 以 uint64 透出。
class AcDbIndexUpdateDataIterator;

class PyDbIndexUpdateDataIterator {
	AcDbIndexUpdateDataIterator* m_p = nullptr;
public:
	// 指针接管：析构 delete。
	explicit PyDbIndexUpdateDataIterator(AcDbIndexUpdateDataIterator* p) : m_p(p) {}
	~PyDbIndexUpdateDataIterator();
	PyDbIndexUpdateDataIterator(const PyDbIndexUpdateDataIterator&) = delete;
	PyDbIndexUpdateDataIterator& operator=(const PyDbIndexUpdateDataIterator&) = delete;

	bool done();
	bool step();

public:
	AcDbIndexUpdateDataIterator* impObj() { return m_p; }
};

void bind_PyDbIndexUpdateDataIterator(pym& m);
