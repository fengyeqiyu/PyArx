#pragma once
#include "stdafx.h"
#include "PyDbSymbolTable.h"
#include "PyDbSymbolTableRecord.h"

class PyDbRegAppTable : public PyDbSymbolTable {
public:
	PyWrapper_H(DbRegAppTable)
	PyDbRegAppTable(AcDbRegAppTable* ptr) : PyDbSymbolTable(ptr) {}
	bool has(const AcString& name) const;
	PyDbSymbolTableRecord getAt(const AcString& name) const;

	// 追加一条记录并返回其 ObjectId（旧 Id，失败返回 0）；
	// 对应 Acad::ErrorStatus add(AcDbObjectId&, AcDbRegAppTableRecord*)
	uint64_t addReturnId(PyDbSymbolTableRecord& rec);
};

void bind_PyDbRegAppTable(pym& m);
