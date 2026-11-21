#pragma once
#include "stdafx.h"
#include "PyDbObject.h"
#include <dbunderlaydef.h>

class PyDbPdfDefinition : public PyDbObject {
public:
	PyWrapper_H(DbPdfDefinition)
	PyDbPdfDefinition(AcDbPdfDefinition* p) : PyDbObject(p) {}
	PyDbPdfDefinition() : PyDbPdfDefinition(new AcDbPdfDefinition()) {}
};

void bind_PyDbPdfDefinition(pym& m);
