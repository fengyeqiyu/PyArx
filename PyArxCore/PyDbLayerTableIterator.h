#pragma once
#include "stdafx.h"
#include "PyDbSymbolTableIterator.h"

class AcDbLayerTableIterator;
class PyDbLayerTable;

// AcDbLayerTableIterator（SDK 头 dbsymtb.h：class AcDbLayerTableIterator : public AcDbSymbolTableIterator）
class PyDbLayerTableIterator : public PyDbSymbolTableIterator {
public:
	PyDbLayerTableIterator(PyDbLayerTable& owner, bool atBeginning = true, bool skipDeleted = true);
	explicit PyDbLayerTableIterator(AcDbLayerTableIterator* p) : PyDbSymbolTableIterator(p) {}
};

void bind_PyDbLayerTableIterator(pym& m);
