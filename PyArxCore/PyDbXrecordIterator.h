#pragma once
#include "stdafx.h"

// SDK: dbxrecrd.h — class AcDbXrecordIterator : public AcRxObject
// 持有者 PyDbXrecord 已封装（P1）。SDK 无 newIterator，直接 new AcDbXrecordIterator(owner.impObj())。
// 当前元素为 resbuf，PyResbuf 尚未封装；暂以 curRestype()(int, DXF 组码)透出，完整 resval 待后续。
class AcDbXrecord;
class AcDbXrecordIterator;

class PyDbXrecordIterator {
	AcDbXrecordIterator* m_p = nullptr;
public:
	// 从持有者创建（本包装接管所有权，析构 delete）。
	explicit PyDbXrecordIterator(AcDbXrecord* pOwner);
	// 指针接管：析构 delete。
	explicit PyDbXrecordIterator(AcDbXrecordIterator* p) : m_p(p) {}
	~PyDbXrecordIterator();
	PyDbXrecordIterator(const PyDbXrecordIterator&) = delete;
	PyDbXrecordIterator& operator=(const PyDbXrecordIterator&) = delete;

	bool done();
	bool step();

public:
	AcDbXrecordIterator* impObj() { return m_p; }
};

void bind_PyDbXrecordIterator(pym& m);
