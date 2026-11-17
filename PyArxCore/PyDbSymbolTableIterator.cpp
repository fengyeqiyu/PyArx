#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSymbolTableIterator.h"
#include "PyDbSymbolTableRecord.h"

void bind_PyDbSymbolTableIterator(pym&) {
	// 公开迭代器已退役；表遍历由 PyDbSymbolTable 的 __iter__/keys/values/items/__getitem__ 覆盖。
}
