#pragma once
#include "stdafx.h"
#include "PyDbObject.h"

// AcDbColor : public AcDbObject   (dbmain.h, line 1731)
// 注：任务简报称 SDK 基类为 AcCmColor；经核对 dbmain.h 实际声明为
//     `class AcDbColor : public AcDbObject`。AcDbColor 内部持有 AcCmColor 成员
//     （m_color），并非继承自 AcCmColor。因此沿用 PyDbObject 标准宏模式。
class PyDbColor : public PyDbObject {
public:
	PyWrapper_H(DbColor)
	PyDbColor(AcDbColor* p) : PyDbObject(p) {}
};

void bind_PyDbColor(pym& m);
