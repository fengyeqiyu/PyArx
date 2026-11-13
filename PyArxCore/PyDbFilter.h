#pragma once
#include "stdafx.h"
#include "PyDbObject.h"

// AcDbFilter : public AcDbObject   (dbfilter.h, line 27)
class PyDbFilter : public PyDbObject {
public:
	PyWrapper_H(DbFilter)
	PyDbFilter(AcDbFilter* p) : PyDbObject(p) {}
};

void bind_PyDbFilter(pym& m);
