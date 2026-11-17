#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbRegAppTableIterator.h"
#include "PyDbRegAppTable.h"
#include "PyDbRegAppTableRecord.h"

PyDbRegAppTableIterator::PyDbRegAppTableIterator(PyDbRegAppTable& owner, bool atBeginning, bool skipDeleted)
	: PyDbSymbolTableIterator(nullptr) {
	AcDbRegAppTableIterator* p = nullptr;
	if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted), L"AcDbRegAppTable::newIterator"))
		p = nullptr;
	m_pObj = p;
}

void bind_PyDbRegAppTableIterator(pym&) {
	// 公开迭代器已退役；RegApp 表遍历由 PyDbSymbolTable 容器协议覆盖。
}
