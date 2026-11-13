#pragma once
#include "stdafx.h"
#include "PyDbObject.h"
#include "PyDbSymbolTableRecord.h"

class PyDbSymbolTable : public PyDbObject {
public:
	PyWrapper_H(DbSymbolTable)
	PyDbSymbolTable(AcDbSymbolTable* ptr) : PyDbObject(ptr) {}
	bool has(const AcString& name) const;
	PyDbSymbolTableRecord getAt(const AcString& name) const;
	py::dict toDict() const;
	py::list names() const;
	py::list records() const;  // 返回所有记录列表
	int numRecords() const;

	// C++ 范围 for 循环支持：begin() / end()
	std::vector<PyDbSymbolTableRecord> toVector() const;

	// 按名称取记录 ObjectId（不打开记录）；失败返回 0
	uint64_t getIdAt(const AcString& name) const;

	// 向符号表追加一条记录（SDK add(AcDbSymbolTableRecord*)）
	bool add(PyDbSymbolTableRecord& rec);
};

void bind_PyDbSymbolTable(pym& m);
