#pragma once
#include "stdafx.h"
#include "PyDbSymbolTableIterator.h"

class AcDbLinetypeTableIterator;
class PyDbLinetypeTable;

// AcDbLinetypeTableIterator（SDK 头 dbsymtb.h：class AcDbLinetypeTableIterator : public AcDbSymbolTableIterator）
class PyDbLinetypeTableIterator : public PyDbSymbolTableIterator {
public:
	PyDbLinetypeTableIterator(PyDbLinetypeTable& owner, bool atBeginning = true, bool skipDeleted = true);
	explicit PyDbLinetypeTableIterator(AcDbLinetypeTableIterator* p) : PyDbSymbolTableIterator(p) {}
};

void bind_PyDbLinetypeTableIterator(pym& m);
