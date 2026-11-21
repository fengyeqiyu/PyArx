#pragma once
#include "stdafx.h"
#include "PyDbObject.h"

// AcDbPlaceHolder : public AcDbObject   (dbplhldr.h, line 19)
class PyDbPlaceHolder : public PyDbObject {
public:
	PyWrapper_H(DbPlaceHolder)
	PyDbPlaceHolder(AcDbPlaceHolder* p) : PyDbObject(p) {}
	PyDbPlaceHolder() : PyDbPlaceHolder(new AcDbPlaceHolder()) {}
};

void bind_PyDbPlaceHolder(pym& m);
