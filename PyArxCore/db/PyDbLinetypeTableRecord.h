#pragma once
#include "stdafx.h"
#include "PyDbSymbolTableRecord.h"

class PyDbLinetypeTableRecord : public PyDbSymbolTableRecord {
public:
	PyWrapper_H(DbLinetypeTableRecord)
	PyDbLinetypeTableRecord(AcDbLinetypeTableRecord* p) : PyDbSymbolTableRecord(p) {}
	PyDbLinetypeTableRecord() : PyDbLinetypeTableRecord(new AcDbLinetypeTableRecord()) {}
};

void bind_PyDbLinetypeTableRecord(pym& m);
