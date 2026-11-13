#pragma once
#include "stdafx.h"
#include "PyDbCurve.h"

class PyDbArc : public PyDbCurve {
	PyWrapper_H(DbArc)
public:
	PyDbArc(AcDbArc* p) : PyDbCurve(p) {}
	PyDbArc() : PyDbArc(new AcDbArc()) {}
	PyDbArc(const AcGePoint3d& center, double radius, double startAngle, double endAngle)
		: PyDbCurve(new AcDbArc(center, radius, startAngle, endAngle)) {}
	PyDbArc(const AcGePoint3d& center, const AcGeVector3d& normal, double radius, double startAngle, double endAngle)
		: PyDbCurve(new AcDbArc(center, normal, radius, startAngle, endAngle)) {}
	AcGePoint3d center() const;
	bool setCenter(const AcGePoint3d&);
	double radius() const;
	bool setRadius(double);
	double startAngle() const;
	bool setStartAngle(double);
	double endAngle() const;
	bool setEndAngle(double);
	double totalAngle() const;
	double length() const;
	double thickness() const;
	bool setThickness(double);
	AcGeVector3d normal() const;
	bool setNormal(const AcGeVector3d&);
	AcString toString() const;
};

void bind_PyDbArc(pym& m);
