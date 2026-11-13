#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

class PyDbSolid : public PyDbEntity {
	PyWrapper_H(DbSolid)
public:
	PyDbSolid(AcDbSolid* p) : PyDbEntity(p) {}
	PyDbSolid() : PyDbSolid(new AcDbSolid()) {}
	PyDbSolid(const AcGePoint3d& pt0, const AcGePoint3d& pt1, const AcGePoint3d& pt2)
		: PyDbEntity(new AcDbSolid(pt0, pt1, pt2)) {}
	PyDbSolid(const AcGePoint3d& pt0, const AcGePoint3d& pt1, const AcGePoint3d& pt2, const AcGePoint3d& pt3)
		: PyDbEntity(new AcDbSolid(pt0, pt1, pt2, pt3)) {}
	AcGePoint3d getPointAt(Adesk::UInt16 index) const;
	bool setPointAt(Adesk::UInt16 index, const AcGePoint3d&);
	double thickness() const;
	bool setThickness(double);
	AcGeVector3d normal() const;
	bool setNormal(const AcGeVector3d&);
	AcString toString() const;
};

void bind_PyDbSolid(pym& m);
