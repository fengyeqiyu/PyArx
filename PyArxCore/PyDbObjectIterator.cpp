#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbObjectIterator.h"

AcDbObjectIterator* PyDbObjectIterator::impObj() { return static_cast<AcDbObjectIterator*>(m_ptr); }
AcDbObjectIterator* PyDbObjectIterator::impObj() const { return static_cast<AcDbObjectIterator*>(m_ptr); }

void bind_PyDbObjectIterator(pym&) {
	// 公开迭代器已退役；全库遍历折叠进 PyDbDatabase.objectIds()（内部类保留）。
}
