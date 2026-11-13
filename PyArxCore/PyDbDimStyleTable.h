#pragma once
#include "stdafx.h"
#include "PyDbSymbolTable.h"
#include "PyDbSymbolTableRecord.h"

class PyDbDimStyleTable : public PyDbSymbolTable {
public:
	PyWrapper_H(DbDimStyleTable)
	PyDbDimStyleTable(AcDbDimStyleTable* ptr) : PyDbSymbolTable(ptr) {}
	bool has(const AcString& name) const;
	PyDbSymbolTableRecord getAt(const AcString& name) const;
};

void bind_PyDbDimStyleTable(pym& m);
