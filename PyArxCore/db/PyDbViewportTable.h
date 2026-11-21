#pragma once
#include "stdafx.h"
#include "PyDbSymbolTable.h"
#include "PyDbSymbolTableRecord.h"

class PyDbViewportTable : public PyDbSymbolTable {
public:
	PyWrapper_H(DbViewportTable)
	PyDbViewportTable(AcDbViewportTable* ptr) : PyDbSymbolTable(ptr) {}
	PyDbViewportTable() : PyDbViewportTable(new AcDbViewportTable()) {}
	bool has(const AcString& name) const;
	PyDbSymbolTableRecord* getAt(const AcString& name, int mode = 1) const;
	// 内部消费 SDK newIterator：遍历返回全部记录 ObjectId（uint64）列表，失败返回空列表
	std::vector<uint64_t> newIterator(bool atBeginning, bool skipDeleted) const;
};

void bind_PyDbViewportTable(pym& m);
