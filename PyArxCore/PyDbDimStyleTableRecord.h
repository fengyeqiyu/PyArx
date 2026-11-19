#pragma once
#include "stdafx.h"
#include "PyDbSymbolTableRecord.h"

class PyDbDimStyleTableRecord : public PyDbSymbolTableRecord {
public:
	PyWrapper_H(DbDimStyleTableRecord)
	PyDbDimStyleTableRecord(AcDbDimStyleTableRecord* p) : PyDbSymbolTableRecord(p) {}
	PyDbDimStyleTableRecord() : PyDbDimStyleTableRecord(new AcDbDimStyleTableRecord()) {}
};

void bind_PyDbDimStyleTableRecord(pym& m);
