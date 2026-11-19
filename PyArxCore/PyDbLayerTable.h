#pragma once
#include "stdafx.h"
#include "PyDbSymbolTable.h"
#include "PyDbLayerTableRecord.h"

class PyDbLayerTable : public PyDbSymbolTable {
public:
	PyWrapper_H(DbLayerTable)
	PyDbLayerTable(AcDbLayerTable* ptr) : PyDbSymbolTable(ptr) {}
	bool has(const AcString& name) const;
	PyDbLayerTableRecord* getAt(const AcString& name, int mode = 1) const;
	bool hasUnreconciledLayers() const;
	void generateUsageData();

	// 返回未协调层的 ObjectId 数组（由 PyArrayCaster 自动转 Python uint64 列表），
	// 失败返回空数组；对应 Acad::ErrorStatus getUnreconciledLayers(AcDbObjectIdArray&) const
	AcDbObjectIdArray getUnreconciledLayers() const;
};

void bind_PyDbLayerTable(pym& m);
