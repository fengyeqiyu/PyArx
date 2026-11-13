#pragma once
#include "stdafx.h"
#include "PyDbSpline.h"
#include <dbhelix.h>

class PyDbHelix : public PyDbSpline {
	PyWrapper_H(DbHelix)
public:
	PyDbHelix(AcDbHelix* p) : PyDbSpline(p) {}
	PyDbHelix() : PyDbHelix(new AcDbHelix()) {}
	double height() const;
	bool setHeight(double);
	double baseRadius() const;
	bool setBaseRadius(double);
	double topRadius() const;
	bool setTopRadius(double);
	double turns() const;
	bool setTurns(double);
	AcGePoint3d startPoint() const;
	bool setStartPoint(const AcGePoint3d&);
	bool createHelix();
	AcGePoint3d axisPoint() const;
	bool setAxisPoint(const AcGePoint3d&, bool bMoveStartPoint = true);
	AcGeVector3d axisVector() const;
	bool setAxisVector(const AcGeVector3d&);
	double turnHeight() const;
	bool setTurnHeight(double);
	bool twist() const;
	bool setTwist(bool);
	int constrain() const;
	bool setConstrain(int);
	double turnSlope() const;
	double totalLength() const;
	AcString toString() const;
};

void bind_PyDbHelix(pym& m);
