#pragma once
#include "stdafx.h"
#include "dbltrans.h"
#include "PyDbObject.h"

// AcDbLongTransaction : public AcDbObject  (dbltrans.h:43)
class PyDbLongTransaction : public PyDbObject {
	PyWrapper_H(DbLongTransaction)
public:
	PyDbLongTransaction(AcDbLongTransaction* p) : PyDbObject(p) {}
};

void bind_PyDbLongTransaction(pym& m);
