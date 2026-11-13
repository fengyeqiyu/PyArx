#pragma once
#include "stdafx.h"
#include <dbVisualStyle.h>
#include "PyDbObject.h"

class PyDbVisualStyle : public PyDbObject {
public:
	PyWrapper_H(DbVisualStyle)
	PyDbVisualStyle(AcDbVisualStyle* ptr) : PyDbObject(ptr) {}

	int type() const;
	bool setType(int type);
	bool setDescription(const AcString& s);
	bool setTraitInt(int prop, int nVal, int op);
	bool setTraitBool(int prop, bool bVal, int op);
	bool setTraitDouble(int prop, double dVal, int op);
	bool setTraitColorF(int prop, double r, double g, double b, int op);
};

void bind_PyDbVisualStyle(pym& m);
