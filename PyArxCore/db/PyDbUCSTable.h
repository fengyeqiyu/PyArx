#pragma once
#include "stdafx.h"
#include "PyDbSymbolTable.h"
#include "PyDbSymbolTableRecord.h"

class PyDbUCSTable : public PyDbSymbolTable {
public:
	PyWrapper_H(DbUCSTable)
	PyDbUCSTable(AcDbUCSTable* ptr) : PyDbSymbolTable(ptr) {}
	PyDbUCSTable() : PyDbUCSTable(new AcDbUCSTable()) {}
	bool has(const AcString& name) const;
	PyDbSymbolTableRecord* getAt(const AcString& name, int mode = 1) const;
};

void bind_PyDbUCSTable(pym& m);
