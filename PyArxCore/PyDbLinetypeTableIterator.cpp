#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLinetypeTableIterator.h"
#include "PyDbLinetypeTable.h"
#include "PyDbLinetypeTableRecord.h"

PyDbLinetypeTableIterator::PyDbLinetypeTableIterator(PyDbLinetypeTable& owner, bool atBeginning, bool skipDeleted)
	: PyDbSymbolTableIterator(nullptr) {
	AcDbLinetypeTableIterator* p = nullptr;
	if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted), L"AcDbLinetypeTable::newIterator"))
		p = nullptr;
	m_pObj = p;
}

void bind_PyDbLinetypeTableIterator(pym&) {
	// 公开迭代器已退役；线型表遍历由 PyDbSymbolTable 容器协议覆盖。
}
