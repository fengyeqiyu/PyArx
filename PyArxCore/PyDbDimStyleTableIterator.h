#pragma once
#include "stdafx.h"
#include "PyDbSymbolTableIterator.h"

class AcDbDimStyleTableIterator;
class PyDbDimStyleTable;

// AcDbDimStyleTableIterator（SDK 头 dbsymtb.h：class AcDbDimStyleTableIterator : public AcDbSymbolTableIterator）
class PyDbDimStyleTableIterator : public PyDbSymbolTableIterator {
public:
	PyDbDimStyleTableIterator(PyDbDimStyleTable& owner, bool atBeginning = true, bool skipDeleted = true);
	explicit PyDbDimStyleTableIterator(AcDbDimStyleTableIterator* p) : PyDbSymbolTableIterator(p) {}
};

void bind_PyDbDimStyleTableIterator(pym& m);
