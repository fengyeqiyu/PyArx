#pragma once
#include "stdafx.h"
#include "PyDbCurve.h"

class PyDbRay : public PyDbCurve {
	PyWrapper_H(DbRay)
public:
	PyDbRay(AcDbRay* p) : PyDbCurve(p) {}
	PyDbRay() : PyDbRay(new AcDbRay()) {}
	AcGePoint3d basePoint() const;
	void setBasePoint(const AcGePoint3d&);
	AcGeVector3d unitDir() const;
	void setUnitDir(const AcGeVector3d&);
	AcGePoint3d secondPoint() const;
	bool setSecondPoint(const AcGePoint3d&);
	AcString toString() const;
};

void bind_PyDbRay(pym& m);
