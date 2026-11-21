#pragma once
#include "stdafx.h"
#include <dbModelDocViewStyle.h>
#include "PyDbObject.h"

class PyDbDatabase;

class PyDbModelDocViewStyle : public PyDbObject {
public:
	PyWrapper_H(DbModelDocViewStyle)
	PyDbModelDocViewStyle(AcDbModelDocViewStyle* ptr) : PyDbObject(ptr) {}

	AcString defaultViewName(int index) const;
	AcString getDescription() const;
	AcString getName() const;
	bool isModifiedForRecompute() const;
	bool isViewNameValid(const AcString& name) const;
	std::tuple<bool, uint64_t> postViewStyleToDb(PyDbDatabase& db, const AcString& styleName);
	bool setDatabaseDefaults(PyDbDatabase& db);
	bool setDescription(const AcString& desc);
	bool setName(const AcString& name);
};

void bind_PyDbModelDocViewStyle(pym& m);
