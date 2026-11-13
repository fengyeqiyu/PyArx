#pragma once
#include "stdafx.h"
#include "PyDbSymbolTableRecord.h"

class PyDbLinetypeTableRecord : public PyDbSymbolTableRecord {
public:
	PyWrapper_H(DbLinetypeTableRecord)
	PyDbLinetypeTableRecord(AcDbLinetypeTableRecord* p) : PyDbSymbolTableRecord(p) {}
};

void bind_PyDbLinetypeTableRecord(pym& m);
