#pragma once
#include "stdafx.h"
#include "PyDbObject.h"

class PyDbEvalVariant;

// AcDbEvalExpr : public AcDbObject  (dbeval.h:1551)
class PyDbEvalExpr : public PyDbObject {
	PyWrapper_H(DbEvalExpr)
public:
	PyDbEvalExpr(AcDbEvalExpr* p) : PyDbObject(p) {}
};

void bind_PyDbEvalExpr(pym& m);
