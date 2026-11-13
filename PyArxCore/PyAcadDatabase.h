#pragma once
#include "stdafx.h"
#include "dbapserv.h"

class PyDbDatabase;

// PyAcadDatabase: host-services accessor for the current/working database
// (acdbHostApplicationServices()->workingDatabase()). There is no separate
// "AcadDatabase" SDK class; this is a thin static facade over the engine's
// working AcDbDatabase, wrapped NON-OWNING (never deleted here).
class PyAcadDatabase {
public:
	// Current working (active) database; non-owning wrapper. Null when the
	// host services / working database is unavailable (e.g. context mode).
	static PyDbDatabase current();

	static bool isAvailable();
	AcString toString() const;
};

void bind_PyAcadDatabase(pym& m);
