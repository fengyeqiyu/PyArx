#pragma once
#include "stdafx.h"

// SDK: dbspindx.h — class AcDbSpatialIndexIterator : public AcDbFilteredBlockIterator
// ctor 需要 AcDbSpatialIndex* + AcDbSpatialFilter*，源类在③档未封装，暂无公开创建路径。
// 迭代语义同基类 AcDbFilteredBlockIterator：start() 起始，id() 当前 id，next() 前进并返回下一 id。
// 当前元素 id() 以 uint64 透出。
class AcDbSpatialIndexIterator;

class PyDbSpatialIndexIterator {
	AcDbSpatialIndexIterator* m_p = nullptr;
public:
	// 指针接管：析构 delete。
	explicit PyDbSpatialIndexIterator(AcDbSpatialIndexIterator* p) : m_p(p) {}
	~PyDbSpatialIndexIterator();
	PyDbSpatialIndexIterator(const PyDbSpatialIndexIterator&) = delete;
	PyDbSpatialIndexIterator& operator=(const PyDbSpatialIndexIterator&) = delete;

	bool done();
	bool step();

public:
	AcDbSpatialIndexIterator* impObj() { return m_p; }
};

void bind_PyDbSpatialIndexIterator(pym& m);
