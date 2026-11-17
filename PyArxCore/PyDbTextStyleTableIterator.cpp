#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbTextStyleTableIterator.h"
#include "PyDbTextStyleTable.h"
#include "PyDbTextStyleTableRecord.h"

PyDbTextStyleTableIterator::PyDbTextStyleTableIterator(PyDbTextStyleTable& owner, bool atBeginning, bool skipDeleted)
	: PyDbSymbolTableIterator(nullptr) {
	AcDbTextStyleTableIterator* p = nullptr;
	if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted), L"AcDbTextStyleTable::newIterator"))
		p = nullptr;
	m_pObj = p;
}

void bind_PyDbTextStyleTableIterator(pym&) {
	// 公开迭代器已退役；文字样式表遍历由 PyDbSymbolTable 容器协议覆盖。
}
