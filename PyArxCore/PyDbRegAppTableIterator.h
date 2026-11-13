#pragma once
#include "stdafx.h"
#include "PyDbSymbolTableIterator.h"

class AcDbRegAppTableIterator;
class PyDbRegAppTable;

// AcDbRegAppTableIterator（SDK 头 dbsymtb.h：class AcDbRegAppTableIterator : public AcDbSymbolTableIterator）
class PyDbRegAppTableIterator : public PyDbSymbolTableIterator {
public:
	PyDbRegAppTableIterator(PyDbRegAppTable& owner, bool atBeginning = true, bool skipDeleted = true);
	explicit PyDbRegAppTableIterator(AcDbRegAppTableIterator* p) : PyDbSymbolTableIterator(p) {}
};

void bind_PyDbRegAppTableIterator(pym& m);
