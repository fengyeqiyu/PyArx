#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

// AcDbBlockBegin : AcDbEntity  (dbents.h:435)
// SDK 仅提供构造/析构，无业务方法（BlockTableRecord 的起始标记）。
class PyDbBlockBegin : public PyDbEntity {
public:
	PyWrapper_H(DbBlockBegin)
	PyDbBlockBegin(AcDbBlockBegin* p) : PyDbEntity(p) {}
};

void bind_PyDbBlockBegin(pym& m);
