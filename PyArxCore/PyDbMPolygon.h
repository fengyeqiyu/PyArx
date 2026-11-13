#pragma once
#include "stdafx.h"
#include "dbMPolygon.h"
#include "PyDbEntity.h"

// AcDbMPolygon has no standalone import library: pointer-only wrapper.
class PyDbMPolygon : public PyDbEntity {
public:
	PyDbMPolygon(AcDbMPolygon* p) : PyDbEntity(p) {}
	PyDbMPolygon() : PyDbMPolygon(new AcDbMPolygon()) {}
	double getArea() const;
	AcString toString() const;
};

void bind_PyDbMPolygon(pym& m);
