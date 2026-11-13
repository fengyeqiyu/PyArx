#pragma once
#include "stdafx.h"

// SDK: dblyindx.h — class AcDbLayerIndexIterator : public AcDbFilteredBlockIterator
// ctor 需要 AcDbLayerIndex* + AcDbLayerFilter*，源类在③档未封装，暂无公开创建路径。
// 迭代语义同基类 AcDbFilteredBlockIterator：start() 起始，id() 当前 id，next() 前进并返回下一 id。
// 当前元素 id() 以 uint64 透出。
class AcDbLayerIndexIterator;

class PyDbLayerIndexIterator {
	AcDbLayerIndexIterator* m_p = nullptr;
public:
	// 指针接管：析构 delete。
	explicit PyDbLayerIndexIterator(AcDbLayerIndexIterator* p) : m_p(p) {}
	~PyDbLayerIndexIterator();
	PyDbLayerIndexIterator(const PyDbLayerIndexIterator&) = delete;
	PyDbLayerIndexIterator& operator=(const PyDbLayerIndexIterator&) = delete;

	bool done();
	bool step();

public:
	AcDbLayerIndexIterator* impObj() { return m_p; }
};

void bind_PyDbLayerIndexIterator(pym& m);
