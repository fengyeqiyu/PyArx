#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

// AcDbOleFrame : public AcDbFrame   (dbole.h, line 49)
// AcDbFrame    : public AcDbEntity (dbframe.h, line 29)
// 工程无 PyDbFrame 包装，上溯到 PyDbEntity 作为 Py 基类。
class PyDbOleFrame : public PyDbEntity {
public:
	PyWrapper_H(DbOleFrame)
	PyDbOleFrame(AcDbOleFrame* p) : PyDbEntity(p) {}
};

void bind_PyDbOleFrame(pym& m);
