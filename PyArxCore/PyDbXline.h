#pragma once
#include "stdafx.h"
#include "PyDbCurve.h"

class PyDbXline : public PyDbCurve {
	PyWrapper_H(DbXline)
public:
	PyDbXline(AcDbXline* p) : PyDbCurve(p) {}
	PyDbXline() : PyDbXline(new AcDbXline()) {}
	AcGePoint3d basePoint() const;
	void setBasePoint(const AcGePoint3d&);
	AcGeVector3d unitDir() const;
	void setUnitDir(const AcGeVector3d&);
	AcGePoint3d secondPoint() const;
	bool setSecondPoint(const AcGePoint3d&);
	AcString toString() const;
};

void bind_PyDbXline(pym& m);
