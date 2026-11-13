#pragma once
#include "stdafx.h"
#include "PyDbCurve.h"

class PyDbEllipse : public PyDbCurve {
	PyWrapper_H(DbEllipse)
public:
	PyDbEllipse(AcDbEllipse* p) : PyDbCurve(p) {}
	PyDbEllipse() : PyDbEllipse(new AcDbEllipse()) {}
	PyDbEllipse(const AcGePoint3d& center, const AcGeVector3d& unitNormal, const AcGeVector3d& majorAxis,
		double radiusRatio, double startAngle = 0.0, double endAngle = 6.28318530717958647692)
		: PyDbCurve(new AcDbEllipse(center, unitNormal, majorAxis, radiusRatio, startAngle, endAngle)) {}
	AcGePoint3d center() const;
	bool setCenter(const AcGePoint3d&);
	AcGeVector3d normal() const;
	AcGeVector3d majorAxis() const;
	AcGeVector3d minorAxis() const;
	double majorRadius() const;
	bool setMajorRadius(double);
	double minorRadius() const;
	bool setMinorRadius(double);
	double radiusRatio() const;
	bool setRadiusRatio(double);
	double startAngle() const;
	bool setStartAngle(double);
	double endAngle() const;
	bool setEndAngle(double);
	bool setStartParam(double);
	bool setEndParam(double);
	double paramAtAngle(double angle) const;
	double angleAtParam(double param) const;
	bool isNull() const;
	std::tuple<AcGePoint3d, AcGeVector3d, AcGeVector3d, double, double, double> get() const;
	bool set(const AcGePoint3d& center, const AcGeVector3d& unitNormal, const AcGeVector3d& majorAxis, double radiusRatio, double startAngle = 0.0, double endAngle = 6.28318530717958647692);
	AcString toString() const;
};

void bind_PyDbEllipse(pym& m);
