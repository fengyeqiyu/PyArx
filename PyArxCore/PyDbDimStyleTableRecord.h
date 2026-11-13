#pragma once
#include "stdafx.h"
#include "PyDbSymbolTableRecord.h"

class PyDbDimStyleTableRecord : public PyDbSymbolTableRecord {
public:
	PyWrapper_H(DbDimStyleTableRecord)
	PyDbDimStyleTableRecord(AcDbDimStyleTableRecord* p) : PyDbSymbolTableRecord(p) {}
};

void bind_PyDbDimStyleTableRecord(pym& m);
