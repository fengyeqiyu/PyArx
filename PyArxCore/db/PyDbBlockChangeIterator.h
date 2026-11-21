#pragma once
#include "stdafx.h"

// SDK: dbindex.h — class AcDbBlockChangeIterator（无基类，独立类）
// ctor 私有，由引擎内部创建；源类在③档未封装，暂无公开创建路径。
// 当前元素 id() 返回 AcDbObjectId，以 uint64 透出。
class AcDbBlockChangeIterator;

class PyDbBlockChangeIterator {
	AcDbBlockChangeIterator* m_p = nullptr;
public:
	// 非拥有指针：SDK 引擎管理该迭代器生命周期（其析构为 private），本包装不 delete。
	explicit PyDbBlockChangeIterator(AcDbBlockChangeIterator* p) : m_p(p) {}
	~PyDbBlockChangeIterator();
	PyDbBlockChangeIterator(const PyDbBlockChangeIterator&) = delete;
	PyDbBlockChangeIterator& operator=(const PyDbBlockChangeIterator&) = delete;

	bool done();
	bool step();

public:
	AcDbBlockChangeIterator* impObj() { return m_p; }
};

void bind_PyDbBlockChangeIterator(pym& m);
