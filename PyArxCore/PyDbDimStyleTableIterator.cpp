#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDimStyleTableIterator.h"
#include "PyDbDimStyleTable.h"
#include "PyDbDimStyleTableRecord.h"

PyDbDimStyleTableIterator::PyDbDimStyleTableIterator(PyDbDimStyleTable& owner, bool atBeginning, bool skipDeleted)
	: PyDbSymbolTableIterator(nullptr) {
	AcDbDimStyleTableIterator* p = nullptr;
	if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted), L"AcDbDimStyleTable::newIterator"))
		p = nullptr;
	m_pObj = p;
}

void bind_PyDbDimStyleTableIterator(pym&) {
	// 公开迭代器已退役；标注样式表遍历由 PyDbSymbolTable 容器协议覆盖。
}
