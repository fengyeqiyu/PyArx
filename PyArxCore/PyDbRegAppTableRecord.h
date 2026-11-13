#pragma once
#include "stdafx.h"
#include "PyDbSymbolTableRecord.h"

class PyDbRegAppTableRecord : public PyDbSymbolTableRecord {
public:
	PyWrapper_H(DbRegAppTableRecord)
	PyDbRegAppTableRecord(AcDbRegAppTableRecord* ptr) : PyDbSymbolTableRecord(ptr) {}

	// ---- Skeleton (no own public accessors beyond inherited name) ----
	// AcDbRegAppTableRecord adds no public methods; name/setName/isDependent/
	// isResolved/isRenamable are inherited from PyDbSymbolTableRecord.
};

void bind_PyDbRegAppTableRecord(pym& m);
