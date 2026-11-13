#pragma once
#include "stdafx.h"
#include "dbimage.h"
#include "PyDbEntity.h"

// AcDbImage : public AcDbEntity  (dbimage.h:24，光栅图抽象基类)
class PyDbImage : public PyDbEntity {
	PyWrapper_H(DbImage)
public:
	PyDbImage(AcDbImage* p) : PyDbEntity(p) {}
};

void bind_PyDbImage(pym& m);
