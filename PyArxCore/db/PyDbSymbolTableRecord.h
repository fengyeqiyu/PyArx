#pragma once
#include "stdafx.h"
#include "PyDbObject.h"
#include "PyDbEntity.h"
#include "PyDbDatabase.h"

class PyDbSymbolTableRecord : public PyDbObject {
public:
	PyWrapper_H(DbSymbolTableRecord)
	PyDbSymbolTableRecord(AcDbSymbolTableRecord* ptr) : PyDbObject(ptr) {}
	bool setName(const AcString& name);
	AcString getName() const;
	bool isDependent() const;
	bool isResolved() const;
	bool isRenamable() const;
	bool upgradeOpen();  // 将对象打开模式从读升级为写（kForRead -> kForWrite）
};

void bind_PyDbSymbolTableRecord(pym& m);
