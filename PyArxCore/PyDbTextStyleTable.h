#pragma once
#include "stdafx.h"
#include "PyDbSymbolTable.h"
#include "PyDbSymbolTableRecord.h"

class PyDbTextStyleTable : public PyDbSymbolTable {
public:
	PyWrapper_H(DbTextStyleTable)
	PyDbTextStyleTable(AcDbTextStyleTable* ptr) : PyDbSymbolTable(ptr) {}
	PyDbTextStyleTable() : PyDbTextStyleTable(new AcDbTextStyleTable()) {}
	bool has(const AcString& name) const;
	PyDbSymbolTableRecord* getAt(const AcString& name, int mode = 1) const;
};

void bind_PyDbTextStyleTable(pym& m);
