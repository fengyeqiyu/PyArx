#pragma once
#include "stdafx.h"
#include "dbObjectContextManager.h"

// AcDbObjectContextManager pointer wrapper (dbObjectContextManager.h; engine-owned
// via AcDbDatabase::objectContextManager(), NOT auto-deleted here).
// registerContextCollection / contextCollection take/return raw
// AcDbObjectContextCollection* which is not yet wrapped; only the name-based
// unregister is exposed here.
class PyDbObjectContextManager {
	AcDbObjectContextManager* m_ptr = nullptr;
public:
	explicit PyDbObjectContextManager(AcDbObjectContextManager* p = nullptr) : m_ptr(p) {}
	AcDbObjectContextManager* impObj() const { return m_ptr; }
	bool isNull() const { return m_ptr == nullptr; }

	bool unregisterContextCollection(const AcString& collectionName);

	AcString toString() const;
};

void bind_PyDbObjectContextManager(pym& m);
