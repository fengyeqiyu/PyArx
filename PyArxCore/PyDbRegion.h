#pragma once
#include "stdafx.h"
#include "dbregion.h"
#include "gepnt2d.h"
#include "gevec2d.h"
#include "PyDbEntity.h"
#include "PyDbCurve.h"

class PyDbRegion : public PyDbEntity {
	PyWrapper_H(DbRegion)
public:
	PyDbRegion(AcDbRegion* p) : PyDbEntity(p) {}
	PyDbRegion() : PyDbRegion(new AcDbRegion()) {}
	double area() const;
	double perimeter() const;
	AcGeVector3d normal() const;
	bool isNull() const;
	unsigned int numChanges() const;
	// (centroid, area, perimeter, momInertia0, momInertia1, prodInertia,
	//  prinMoment0, prinMoment1, prinAxis0, prinAxis1, radiusGyration0, radiusGyration1,
	//  extentsLow, extentsHigh)
	std::tuple<AcGePoint2d, double, double,
		double, double, double,
		double, double,
		AcGeVector2d, AcGeVector2d,
		double, double,
		AcGePoint2d, AcGePoint2d> getAreaProp() const;
	std::tuple<AcGePlane, int> getPlane() const;
	bool booleanOper(int operation, PyDbRegion& other);
	bool usesGraphicsCache() const;
	static std::vector<PyDbRegion> createFromCurves(const std::vector<PyDbCurve*>& curves);
	AcString toString() const;
};

void bind_PyDbRegion(pym& m);
