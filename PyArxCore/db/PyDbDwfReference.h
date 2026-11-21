#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"
#include <dbunderlayref.h>

class PyDbDwfReference : public PyDbEntity {
public:
	PyWrapper_H(DbDwfReference)
	PyDbDwfReference(AcDbDwfReference* p) : PyDbEntity(p) {}
	PyDbDwfReference() : PyDbDwfReference(new AcDbDwfReference()) {}
};

void bind_PyDbDwfReference(pym& m);
