#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

class PyDbPoint : public PyDbEntity {
	PyWrapper_H(DbPoint)
public:
	PyDbPoint(AcDbPoint* p) : PyDbEntity(p) {}
	PyDbPoint() : PyDbPoint(new AcDbPoint()) {}
	PyDbPoint(const AcGePoint3d& position) : PyDbEntity(new AcDbPoint(position)) {}
	AcGePoint3d position() const;
	bool setPosition(const AcGePoint3d&);
	double thickness() const;
	bool setThickness(double);
	AcGeVector3d normal() const;
	bool setNormal(const AcGeVector3d&);
	double ecsRotation() const;
	bool setEcsRotation(double);
	AcString toString() const;
};

void bind_PyDbPoint(pym& m);
