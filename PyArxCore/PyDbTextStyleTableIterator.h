#pragma once
#include "stdafx.h"
#include "PyDbSymbolTableIterator.h"

class AcDbTextStyleTableIterator;
class PyDbTextStyleTable;

// AcDbTextStyleTableIterator（SDK 头 dbsymtb.h：class AcDbTextStyleTableIterator : public AcDbSymbolTableIterator）
class PyDbTextStyleTableIterator : public PyDbSymbolTableIterator {
public:
	PyDbTextStyleTableIterator(PyDbTextStyleTable& owner, bool atBeginning = true, bool skipDeleted = true);
	explicit PyDbTextStyleTableIterator(AcDbTextStyleTableIterator* p) : PyDbSymbolTableIterator(p) {}
};

void bind_PyDbTextStyleTableIterator(pym& m);
