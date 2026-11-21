#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

// AcDbBlockEnd : AcDbEntity  (dbents.h:444)
// SDK 仅提供构造/析构，无业务方法（BlockTableRecord 的结束标记）。
class PyDbBlockEnd : public PyDbEntity {
public:
	PyWrapper_H(DbBlockEnd)
	PyDbBlockEnd(AcDbBlockEnd* p) : PyDbEntity(p) {}
	PyDbBlockEnd() : PyDbBlockEnd(new AcDbBlockEnd()) {}
};

void bind_PyDbBlockEnd(pym& m);
