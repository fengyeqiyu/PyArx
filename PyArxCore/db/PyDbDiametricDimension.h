#pragma once
#include "stdafx.h"
#include "PyDbDimension.h"

class PyDbDiametricDimension : public PyDbDimension {
	PyWrapper_H(DbDiametricDimension)
public:
	PyDbDiametricDimension(AcDbDiametricDimension* p) : PyDbDimension(p) {}
	PyDbDiametricDimension() : PyDbDiametricDimension(new AcDbDiametricDimension()) {}
	AcGePoint3d chordPoint() const;
	bool setChordPoint(const AcGePoint3d&);
	AcGePoint3d farChordPoint() const;
	bool setFarChordPoint(const AcGePoint3d&);
	double leaderLength() const;
	bool setLeaderLength(double);
	double extArcStartAngle() const;
	bool setExtArcStartAngle(double);
	double extArcEndAngle() const;
	bool setExtArcEndAngle(double);
	AcString toString() const;
};

void bind_PyDbDiametricDimension(pym& m);
