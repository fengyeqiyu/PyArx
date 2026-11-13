#pragma once
#include "stdafx.h"
#include "PyDbSymbolTable.h"
#include "PyDbSymbolTableRecord.h"

class PyDbViewTable : public PyDbSymbolTable {
public:
	PyWrapper_H(DbViewTable)
	PyDbViewTable(AcDbViewTable* ptr) : PyDbSymbolTable(ptr) {}
	bool has(const AcString& name) const;
	PyDbSymbolTableRecord getAt(const AcString& name) const;
};

void bind_PyDbViewTable(pym& m);
