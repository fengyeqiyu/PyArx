#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

class PyDbMline : public PyDbEntity {
	PyWrapper_H(DbMline)
public:
	PyDbMline(AcDbMline* p) : PyDbEntity(p) {}
	PyDbMline() : PyDbMline(new AcDbMline()) {}
	double scale() const;
	bool setScale(double);
	int numVertices() const;
	AcGePoint3d vertexAt(int index) const;
	bool closedMline() const;
	bool setClosedMline(bool closed);
	int justification() const;
	bool setJustification(int);
	AcGeVector3d normal() const;
	bool setNormal(const AcGeVector3d&);
	bool appendSeg(const AcGePoint3d&);
	bool moveVertexAt(int index, const AcGePoint3d&);
	bool supressStartCaps() const;
	bool setSupressStartCaps(bool);
	bool supressEndCaps() const;
	bool setSupressEndCaps(bool);
	int element(const AcGePoint3d& pt) const;
	AcGeVector3d axisAt(int index) const;
	AcGeVector3d miterAt(int index) const;
	// ---- 新增 ----
	AcGePoint3d getClosestPointTo(const AcGePoint3d& givenPoint, bool extend, bool excludeCaps = false) const;
	AcGePoint3d getClosestPointTo(const AcGePoint3d& givenPoint, const AcGeVector3d& normal, bool extend, bool excludeCaps = false) const;
	AcGePlane getPlane() const;
	AcGePoint3d removeLastSeg();
	uint64_t style() const;
	bool setStyle(uint64_t styleId);
	AcString toString() const;
};

void bind_PyDbMline(pym& m);
