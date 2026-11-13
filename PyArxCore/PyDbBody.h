#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

class PyDbBody : public PyDbEntity {
	PyWrapper_H(DbBody)
public:
	PyDbBody(AcDbBody* p) : PyDbEntity(p) {}
	PyDbBody() : PyDbBody(new AcDbBody()) {}
	bool isNull() const;
	unsigned int numChanges() const;
	void invalidateCachedBodySubentIds();
	bool usesGraphicsCache() const;
	static std::vector<PyDbBody> acisIn(const AcString& fileName);
	static bool acisOut(const AcString& fileName, const std::vector<PyDbBody*>& bodies);
	AcString toString() const;
};

void bind_PyDbBody(pym& m);
