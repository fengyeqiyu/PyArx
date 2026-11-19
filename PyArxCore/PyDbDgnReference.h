#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"
#include <dbunderlayref.h>

class PyDbDgnReference : public PyDbEntity {
public:
	PyWrapper_H(DbDgnReference)
	PyDbDgnReference(AcDbDgnReference* p) : PyDbEntity(p) {}
	PyDbDgnReference() : PyDbDgnReference(new AcDbDgnReference()) {}
};

void bind_PyDbDgnReference(pym& m);
