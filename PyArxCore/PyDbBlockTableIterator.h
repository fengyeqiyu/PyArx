#pragma once
#include "stdafx.h"
#include "PyDbSymbolTableIterator.h"

class AcDbBlockTableIterator;
class PyDbBlockTable;

// AcDbBlockTableIterator（SDK 头 dbsymtb.h：class AcDbBlockTableIterator : public AcDbSymbolTableIterator）
class PyDbBlockTableIterator : public PyDbSymbolTableIterator {
public:
	PyDbBlockTableIterator(PyDbBlockTable& owner, bool atBeginning = true, bool skipDeleted = true);
	explicit PyDbBlockTableIterator(AcDbBlockTableIterator* p) : PyDbSymbolTableIterator(p) {}
};

void bind_PyDbBlockTableIterator(pym& m);
