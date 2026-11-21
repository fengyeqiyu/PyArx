#pragma once
#include "stdafx.h"
#include "PyDbCurve.h"

class PyDbCircle : public PyDbCurve {
	PyWrapper_H(DbCircle)
public:
	PyDbCircle(AcDbCircle* p) : PyDbCurve(p) {}
	PyDbCircle() : PyDbCircle(new AcDbCircle()) {}
	PyDbCircle(const AcGePoint3d& cntr, const AcGeVector3d& nrm, double radius)
		: PyDbCurve(new AcDbCircle(cntr, nrm, radius)) {}
	AcGePoint3d center() const;
	bool setCenter(const AcGePoint3d&);
	double radius() const;
	bool setRadius(double);
	AcGeVector3d normal() const;
	bool setNormal(const AcGeVector3d&);
	double thickness() const;
	bool setThickness(double);
	double circumference() const;
	bool setCircumference(double);
	double diameter() const;
	bool setDiameter(double);
	AcString toString() const;
};

void bind_PyDbCircle(pym& m);
