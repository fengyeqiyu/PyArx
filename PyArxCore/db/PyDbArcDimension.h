#pragma once
#include "stdafx.h"
#include "PyDbDimension.h"

class PyDbArcDimension : public PyDbDimension {
	PyWrapper_H(DbArcDimension)
public:
	PyDbArcDimension(AcDbArcDimension* p) : PyDbDimension(p) {}
	PyDbArcDimension() : PyDbArcDimension(new AcDbArcDimension()) {}
	AcGePoint3d arcPoint() const;
	bool setArcPoint(const AcGePoint3d&);
	AcGePoint3d centerPoint() const;
	bool setCenterPoint(const AcGePoint3d&);
	AcGePoint3d xLine1Point() const;
	bool setXLine1Point(const AcGePoint3d&);
	AcGePoint3d xLine2Point() const;
	bool setXLine2Point(const AcGePoint3d&);
	bool isPartial() const;
	bool setIsPartial(bool partial);
	double arcStartParam() const;
	bool setArcStartParam(double);
	double arcEndParam() const;
	bool setArcEndParam(double);
	bool hasLeader() const;
	bool setHasLeader(bool);
	AcGePoint3d leader1Point() const;
	bool setLeader1Point(const AcGePoint3d&);
	AcGePoint3d leader2Point() const;
	bool setLeader2Point(const AcGePoint3d&);
	int arcSymbolType() const;
	bool setArcSymbolType(int);
	AcString toString() const;
};

void bind_PyDbArcDimension(pym& m);
