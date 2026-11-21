#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

class PyDbTrace : public PyDbEntity {
	PyWrapper_H(DbTrace)
public:
	PyDbTrace(AcDbTrace* p) : PyDbEntity(p) {}
	PyDbTrace() : PyDbTrace(new AcDbTrace()) {}
	PyDbTrace(const AcGePoint3d& pt0, const AcGePoint3d& pt1, const AcGePoint3d& pt2, const AcGePoint3d& pt3)
		: PyDbEntity(new AcDbTrace(pt0, pt1, pt2, pt3)) {}
	AcGePoint3d getPointAt(Adesk::UInt16 index) const;
	bool setPointAt(Adesk::UInt16 index, const AcGePoint3d&);
	double thickness() const;
	bool setThickness(double);
	AcGeVector3d normal() const;
	bool setNormal(const AcGeVector3d&);
	AcString toString() const;
};

void bind_PyDbTrace(pym& m);
