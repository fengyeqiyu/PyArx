#pragma once
#include "stdafx.h"

// SDK: dbltrans.h — class AcDbLongTransWorkSetIterator : public AcRxObject
// 抽象基类，无公开构造；由 AcDbLongTransaction::newWorkSetIterator() 创建。
// 持有者 PyDbLongTransaction 尚未封装（P2 并行在建），当前仅提供指针接管构造，
// 暂无公开创建路径，待源类封装后可用。本类只做 Python 迭代协议。
class AcDbLongTransWorkSetIterator;

class PyDbLongTransWorkSetIterator {
	AcDbLongTransWorkSetIterator* m_p = nullptr;
public:
	// 指针接管：析构 delete。
	explicit PyDbLongTransWorkSetIterator(AcDbLongTransWorkSetIterator* p) : m_p(p) {}
	~PyDbLongTransWorkSetIterator();
	PyDbLongTransWorkSetIterator(const PyDbLongTransWorkSetIterator&) = delete;
	PyDbLongTransWorkSetIterator& operator=(const PyDbLongTransWorkSetIterator&) = delete;

	bool done();
	bool step();

public:
	AcDbLongTransWorkSetIterator* impObj() { return m_p; }
};

void bind_PyDbLongTransWorkSetIterator(pym& m);
