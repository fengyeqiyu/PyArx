#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbBlockTableIterator.h"
#include "PyDbBlockTable.h"
#include "PyDbBlockTableRecord.h"

PyDbBlockTableIterator::PyDbBlockTableIterator(PyDbBlockTable& owner, bool atBeginning, bool skipDeleted)
	: PyDbSymbolTableIterator(nullptr) {
	AcDbBlockTableIterator* p = nullptr;
	if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted), L"AcDbBlockTable::newIterator"))
		p = nullptr;
	m_pObj = p;
}

void bind_PyDbBlockTableIterator(pym&) {
	// 公开迭代器已退役；块表遍历由 PyDbSymbolTable 容器协议覆盖。
}
