#pragma once
#include "stdafx.h"
#include "dbmain.h"

class PyDbDatabase;

// AcDbDatabaseReactor wrapper (trampoline 模式)
// Python 子类继承并重写方法以接收数据库事件回调
class PyDbDatabaseReactor : public AcDbDatabaseReactor {
public:
	PyDbDatabaseReactor() = default;

	// trampoline 虚函数重写
	void objectAppended(const AcDbDatabase* pDb, const AcDbObject* pObj) override;
	void objectUnAppended(const AcDbDatabase* pDb, const AcDbObject* pObj) override;
	void objectReAppended(const AcDbDatabase* pDb, const AcDbObject* pObj) override;
	void objectOpenedForModify(const AcDbDatabase* pDb, const AcDbObject* pObj) override;
	void objectModified(const AcDbDatabase* pDb, const AcDbObject* pObj) override;
	void objectErased(const AcDbDatabase* pDb, const AcDbObject* pObj, bool erased) override;
	void headerSysVarWillChange(const AcDbDatabase* pDb, const wchar_t* name) override;
	void headerSysVarChanged(const AcDbDatabase* pDb, const wchar_t* name, bool success) override;
	void goodbye(const AcDbDatabase* pDb) override;

	// Attach / detach on a database
	bool attach(PyDbDatabase& db);
	bool detach(PyDbDatabase& db);
};

void bind_PyDbDatabaseReactor(pym& m);
