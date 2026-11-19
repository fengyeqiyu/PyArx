#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"
#include <dbunderlayref.h>

class PyDbPdfReference : public PyDbEntity {
public:
	PyWrapper_H(DbPdfReference)
	PyDbPdfReference(AcDbPdfReference* p) : PyDbEntity(p) {}
	PyDbPdfReference() : PyDbPdfReference(new AcDbPdfReference()) {}
};

void bind_PyDbPdfReference(pym& m);
