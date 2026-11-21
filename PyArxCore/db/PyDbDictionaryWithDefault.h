#pragma once
#include "stdafx.h"
#include "PyDbDictionary.h"

// AcDbDictionaryWithDefault : public AcDbDictionary   (dbdictdflt.h, line 22)
class PyDbDictionaryWithDefault : public PyDbDictionary {
public:
	PyWrapper_H(DbDictionaryWithDefault)
	PyDbDictionaryWithDefault(AcDbDictionaryWithDefault* p) : PyDbDictionary(p) {}
	PyDbDictionaryWithDefault() : PyDbDictionaryWithDefault(new AcDbDictionaryWithDefault()) {}
};

void bind_PyDbDictionaryWithDefault(pym& m);
