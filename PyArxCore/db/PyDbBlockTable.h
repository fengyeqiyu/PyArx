#pragma once
#include "stdafx.h"
#include "PyDbSymbolTable.h"
#include "PyDbBlockTableRecord.h"

class PyDbBlockTable : public PyDbSymbolTable {
public:
	PyWrapper_H(DbBlockTable)
	PyDbBlockTable(AcDbBlockTable* ptr) : PyDbSymbolTable(ptr) {}
	PyDbBlockTable() : PyDbBlockTable(new AcDbBlockTable()) {}
	bool has(const AcString& name) const;
	// mode: 0=kForRead, 1=kForWrite（默认 1=写模式直接打开，避免 upgradeOpen 的 eInvalidOpenState 问题）
	PyDbBlockTableRecord* getAt(const AcString& name, int mode = 1) const;
};

void bind_PyDbBlockTable(pym& m);
