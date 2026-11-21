#pragma once
#include "stdafx.h"
#include "PyDbDimension.h"

class PyDbRadialDimensionLarge : public PyDbDimension {
	PyWrapper_H(DbRadialDimensionLarge)
public:
	PyDbRadialDimensionLarge(AcDbRadialDimensionLarge* p) : PyDbDimension(p) {}
	PyDbRadialDimensionLarge() : PyDbRadialDimensionLarge(new AcDbRadialDimensionLarge()) {}
	AcGePoint3d center() const;
	bool setCenter(const AcGePoint3d&);
	AcGePoint3d chordPoint() const;
	bool setChordPoint(const AcGePoint3d&);
	AcGePoint3d overrideCenter() const;
	bool setOverrideCenter(const AcGePoint3d&);
	AcGePoint3d jogPoint() const;
	bool setJogPoint(const AcGePoint3d&);
	double jogAngle() const;
	bool setJogAngle(double a);
	bool setOverrideCenterPP(const AcGePoint3d&);
	bool setJogPointPP(const AcGePoint3d&);
	bool setTextPositionPP(const AcGePoint3d&);
	double extArcStartAngle() const;
	bool setExtArcStartAngle(double);
	double extArcEndAngle() const;
	bool setExtArcEndAngle(double);
	AcString toString() const;
};

void bind_PyDbRadialDimensionLarge(pym& m);
