#pragma once
#include "stdafx.h"
#include "PyDbSymbolTable.h"
#include "PyDbSymbolTableRecord.h"

class PyDbLinetypeTable : public PyDbSymbolTable {
public:
	PyWrapper_H(DbLinetypeTable)
	PyDbLinetypeTable(AcDbLinetypeTable* ptr) : PyDbSymbolTable(ptr) {}
	bool has(const AcString& name) const;
	PyDbSymbolTableRecord* getAt(const AcString& name, int mode = 1) const;
};

void bind_PyDbLinetypeTable(pym& m);
