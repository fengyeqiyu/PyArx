#pragma once
#include "stdafx.h"
#include "PyDbDimension.h"

class PyDb2LineAngularDimension : public PyDbDimension {
	PyWrapper_H(Db2LineAngularDimension)
public:
	PyDb2LineAngularDimension(AcDb2LineAngularDimension* p) : PyDbDimension(p) {}
	PyDb2LineAngularDimension() : PyDb2LineAngularDimension(new AcDb2LineAngularDimension()) {}
	AcGePoint3d arcPoint() const;
	bool setArcPoint(const AcGePoint3d&);
	AcGePoint3d xLine1Start() const;
	bool setXLine1Start(const AcGePoint3d&);
	AcGePoint3d xLine1End() const;
	bool setXLine1End(const AcGePoint3d&);
	AcGePoint3d xLine2Start() const;
	bool setXLine2Start(const AcGePoint3d&);
	AcGePoint3d xLine2End() const;
	bool setXLine2End(const AcGePoint3d&);
	bool extArcOn() const;
	bool setExtArcOn(bool);
	AcString toString() const;
};

void bind_PyDb2LineAngularDimension(pym& m);
