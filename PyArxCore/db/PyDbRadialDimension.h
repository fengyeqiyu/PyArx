#pragma once
#include "stdafx.h"
#include "PyDbDimension.h"

class PyDbRadialDimension : public PyDbDimension {
	PyWrapper_H(DbRadialDimension)
public:
	PyDbRadialDimension(AcDbRadialDimension* p) : PyDbDimension(p) {}
	PyDbRadialDimension() : PyDbRadialDimension(new AcDbRadialDimension()) {}
	AcGePoint3d center() const;
	bool setCenter(const AcGePoint3d&);
	AcGePoint3d chordPoint() const;
	bool setChordPoint(const AcGePoint3d&);
	double leaderLength() const;
	bool setLeaderLength(double);
	double extArcStartAngle() const;
	bool setExtArcStartAngle(double);
	double extArcEndAngle() const;
	bool setExtArcEndAngle(double);
	AcString toString() const;
};

void bind_PyDbRadialDimension(pym& m);
