#pragma once
#include "stdafx.h"

class AcDbSymbolTableIterator;

// AcDbSymbolTableIterator（SDK 头 dbsymtb.h：class AcDbSymbolTableIterator —— 无基类，非 AcRxObject 派生）
// 符号表迭代器抽象基类；由各具体持有者表 newIterator() 创建，本包装接管指针（析构 delete）。
// P2 批次：仅绑 Python 迭代协议（done / step / getRecordId / __iter__ / __next__），不绑其余方法。
// 注意：SDK 中此类与 AcDbObjectIterator 无继承关系，故本包装独立持有指针，不从 PyDbObjectIterator 派生。
class PyDbSymbolTableIterator {
public:
	explicit PyDbSymbolTableIterator(AcDbSymbolTableIterator* p) : m_pObj(p) {}
	virtual ~PyDbSymbolTableIterator() { delete m_pObj; m_pObj = nullptr; }
	PyDbSymbolTableIterator(const PyDbSymbolTableIterator&) = delete;
	PyDbSymbolTableIterator& operator=(const PyDbSymbolTableIterator&) = delete;

	AcDbSymbolTableIterator* impObj() { return m_pObj; }
	const AcDbSymbolTableIterator* impObj() const { return m_pObj; }

protected:
	AcDbSymbolTableIterator* m_pObj;
};

void bind_PyDbSymbolTableIterator(pym& m);
