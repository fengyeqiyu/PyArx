#pragma once
#include "stdafx.h"
#include <dbxrecrd.h>
#include "PyDbObject.h"

// AcDbXrecord : AcDbObject. resbuf chain accessors are left as skeleton
// (raw resbuf* marshalling deferred; PyResbuf belongs to the PyEd module).
class PyDbXrecord : public PyDbObject {
	PyWrapper_H(DbXrecord)
public:
	PyDbXrecord(AcDbXrecord* ptr) : PyDbObject(ptr) {}
	PyDbXrecord() : PyDbObject(new AcDbXrecord()) {}

	bool isXlateReferences() const;
	bool setXlateReferences(bool translate);

	// NOTE: rbChain / setFromRbChain / appendRbChain (resbuf*) deferred.
	AcString toString() const;
};

void bind_PyDbXrecord(pym& m);
