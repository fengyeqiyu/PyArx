#pragma once
#include "stdafx.h"
#include "PyDbObject.h"
#include <dbunderlaydef.h>

class PyDbDwfDefinition : public PyDbObject {
public:
	PyWrapper_H(DbDwfDefinition)
	PyDbDwfDefinition(AcDbDwfDefinition* p) : PyDbObject(p) {}
};

void bind_PyDbDwfDefinition(pym& m);
