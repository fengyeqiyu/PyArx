#pragma once
#include "stdafx.h"
#include "dbtrans.h"
#include <vector>

class PyDbObject;

// AcDbTransactionManager pointer wrapper (dbtrans.h; engine-owned, obtained via
// AcDbDatabase::transactionManager(), NOT auto-deleted here).
// NOTE: this wraps the manager itself; the RAII transaction-context helper
// already shipped as PyDbTrans (PyDbTrans.h/.cpp) is a different concern and
// is NOT duplicated here. AcTransaction (startTransaction/topTransaction) and
// AcTransactionReactor (addReactor/removeReactor) remain unbound: the reactor
// base class has no Python-side wrapper, so Python callers cannot supply one.
class PyDbTransactionManager {
	AcDbTransactionManager* m_ptr = nullptr;
public:
	explicit PyDbTransactionManager(AcDbTransactionManager* p = nullptr) : m_ptr(p) {}
	AcDbTransactionManager* impObj() const { return m_ptr; }
	bool isNull() const { return m_ptr == nullptr; }

	int numActiveTransactions() const;
	int numOpenedObjects() const;
	bool endTransaction();
	bool abortTransaction();
	bool queueForGraphicsFlush();

	// 事务托管的 DB 对象
	bool addNewlyCreatedDBRObject(PyDbObject& obj, bool add = true);
	std::vector<PyDbObject> getAllObjects();
	PyDbObject getObject(uint64_t objId, int mode = 0, bool openErased = false);
	bool markObjectPageable(PyDbObject& obj);
	AcString toString() const;
};

void bind_PyDbTransactionManager(pym& m);
