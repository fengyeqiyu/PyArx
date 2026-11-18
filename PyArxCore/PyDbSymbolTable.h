#pragma once
#include "stdafx.h"
#include "PyDbObject.h"
#include "PyDbSymbolTableRecord.h"

class PyDbSymbolTable : public PyDbObject {
public:
	PyWrapper_H(DbSymbolTable)
	PyDbSymbolTable(AcDbSymbolTable* ptr) : PyDbObject(ptr) {}
	bool has(const AcString& name) const;
	PyDbSymbolTableRecord getAt(const AcString& name, int mode = 1) const;
	py::dict toDict() const;
	py::list names() const;
	py::list records(int mode = 1) const;  // 返回所有记录列表（默认写模式打开）
	py::list keys() const;                 // 记录名列表
	py::list values(int mode = 1) const;   // 记录对象列表
	py::list items(int mode = 1) const;    // (记录名, 记录对象) 列表
	py::object get(const AcString& name, py::object defaultValue = py::none()) const;
	int numRecords() const;

	// C++ 范围 for 循环支持：begin() / end()
	std::vector<PyDbSymbolTableRecord> toVector() const;

	// 按名称取记录 ObjectId（不打开记录）；失败返回 0
	uint64_t getIdAt(const AcString& name) const;

	// 索引访问（负索引从末尾算；越界抛 IndexError）
	PyDbSymbolTableRecord recordAt(int index, int mode = 1) const;
	AcString keyAt(int index) const;
	// 取或建：名字已存在直接返回；不存在则把 rec 追加进表，再按 name 打开返回
	PyDbSymbolTableRecord getOrCreate(const AcString& name, PyDbSymbolTableRecord& rec, int mode = 1);

	// 向符号表追加一条记录（SDK add(AcDbSymbolTableRecord*)）
	bool add(PyDbSymbolTableRecord& rec);
};

void bind_PyDbSymbolTable(pym& m);
