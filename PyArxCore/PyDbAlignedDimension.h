#pragma once
#include "stdafx.h"
#include "PyDbDimension.h"

class PyDbAlignedDimension : public PyDbDimension {
	PyWrapper_H(DbAlignedDimension)
public:
	PyDbAlignedDimension(AcDbAlignedDimension* p) : PyDbDimension(p) {}
	PyDbAlignedDimension() : PyDbAlignedDimension(new AcDbAlignedDimension()) {}
	AcGePoint3d xLine1Point() const;
	bool setXLine1Point(const AcGePoint3d&);
	AcGePoint3d xLine2Point() const;
	bool setXLine2Point(const AcGePoint3d&);
	AcGePoint3d dimLinePoint() const;
	bool setDimLinePoint(const AcGePoint3d&);
	double oblique() const;
	bool setOblique(double);
	bool jogSymbolOn() const;
	bool setJogSymbolOn(bool);
	AcGePoint3d jogSymbolPosition() const;
	bool setJogSymbolPosition(const AcGePoint3d&);
	AcString toString() const;
};

void bind_PyDbAlignedDimension(pym& m);
