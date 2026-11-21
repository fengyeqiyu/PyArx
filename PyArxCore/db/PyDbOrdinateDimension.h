#pragma once
#include "stdafx.h"
#include "PyDbDimension.h"

class PyDbOrdinateDimension : public PyDbDimension {
	PyWrapper_H(DbOrdinateDimension)
public:
	PyDbOrdinateDimension(AcDbOrdinateDimension* p) : PyDbDimension(p) {}
	PyDbOrdinateDimension() : PyDbOrdinateDimension(new AcDbOrdinateDimension()) {}
	AcGePoint3d definingPoint() const;
	bool setDefiningPoint(const AcGePoint3d&);
	AcGePoint3d leaderEndPoint() const;
	bool setLeaderEndPoint(const AcGePoint3d&);
	AcGePoint3d origin() const;
	bool setOrigin(const AcGePoint3d&);
	bool isUsingXAxis() const;
	bool isUsingYAxis() const;
	bool useXAxis();
	bool useYAxis();
	bool setUsingXAxis(bool);
	bool setUsingYAxis(bool);
	AcString toString() const;
};

void bind_PyDbOrdinateDimension(pym& m);
