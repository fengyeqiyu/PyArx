#pragma once
#include "stdafx.h"

// SDK: dbeval.h — class AcDbEvalContextIterator : public AcRxObject, public AcHeapOperators
// 持有者 AcDbEvalContext 在③档未封装，暂无公开创建路径。
// 当前元素 contextPair() 得 AcDbEvalContextPair，以其 key()(宽字符串)透出。
class AcDbEvalContextIterator;

class PyDbEvalContextIterator {
	AcDbEvalContextIterator* m_p = nullptr;
public:
	// 指针接管：析构 delete。
	explicit PyDbEvalContextIterator(AcDbEvalContextIterator* p) : m_p(p) {}
	~PyDbEvalContextIterator();
	PyDbEvalContextIterator(const PyDbEvalContextIterator&) = delete;
	PyDbEvalContextIterator& operator=(const PyDbEvalContextIterator&) = delete;

	bool done();
	bool step();

public:
	AcDbEvalContextIterator* impObj() { return m_p; }
};

void bind_PyDbEvalContextIterator(pym& m);
