#pragma once
#include "stdafx.h"
#include "PyDbDimension.h"

class PyDb3PointAngularDimension : public PyDbDimension {
	PyWrapper_H(Db3PointAngularDimension)
public:
	PyDb3PointAngularDimension(AcDb3PointAngularDimension* p) : PyDbDimension(p) {}
	PyDb3PointAngularDimension() : PyDb3PointAngularDimension(new AcDb3PointAngularDimension()) {}
	AcGePoint3d centerPoint() const;
	bool setCenterPoint(const AcGePoint3d&);
	AcGePoint3d arcPoint() const;
	bool setArcPoint(const AcGePoint3d&);
	AcGePoint3d xLine1Point() const;
	bool setXLine1Point(const AcGePoint3d&);
	AcGePoint3d xLine2Point() const;
	bool setXLine2Point(const AcGePoint3d&);
	bool extArcOn() const;
	bool setExtArcOn(bool);
	AcString toString() const;
};

void bind_PyDb3PointAngularDimension(pym& m);
