#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLayerTableIterator.h"
#include "PyDbLayerTable.h"
#include "PyDbLayerTableRecord.h"

PyDbLayerTableIterator::PyDbLayerTableIterator(PyDbLayerTable& owner, bool atBeginning, bool skipDeleted)
	: PyDbSymbolTableIterator(nullptr) {
	AcDbLayerTableIterator* p = nullptr;
	if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted), L"AcDbLayerTable::newIterator"))
		p = nullptr;
	m_pObj = p;
}

void bind_PyDbLayerTableIterator(pym&) {
	// 公开迭代器已退役；图层表遍历由 PyDbSymbolTable 容器协议覆盖。
}
