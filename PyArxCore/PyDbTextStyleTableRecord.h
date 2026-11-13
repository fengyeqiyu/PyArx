#pragma once
#include "stdafx.h"
#include "PyDbSymbolTableRecord.h"

class PyDbTextStyleTableRecord : public PyDbSymbolTableRecord {
public:
	PyWrapper_H(DbTextStyleTableRecord)
	PyDbTextStyleTableRecord(AcDbTextStyleTableRecord* p) : PyDbSymbolTableRecord(p) {}
};

void bind_PyDbTextStyleTableRecord(pym& m);
