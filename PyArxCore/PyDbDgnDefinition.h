#pragma once
#include "stdafx.h"
#include "PyDbObject.h"
#include <dbunderlaydef.h>

class PyDbDgnDefinition : public PyDbObject {
public:
	PyWrapper_H(DbDgnDefinition)
	PyDbDgnDefinition(AcDbDgnDefinition* p) : PyDbObject(p) {}
};

void bind_PyDbDgnDefinition(pym& m);
