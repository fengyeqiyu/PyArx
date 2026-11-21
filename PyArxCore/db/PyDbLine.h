#pragma once
#include "stdafx.h"
#include "PyDbCurve.h"

class PyDbLine : public PyDbCurve {
	PyWrapper_H(DbLine)
public:
	PyDbLine(AcDbLine* p) : PyDbCurve(p) {}
	PyDbLine() : PyDbLine(new AcDbLine()) {};
	PyDbLine(const AcGePoint3d& pt1, const AcGePoint3d& pt2) : PyDbCurve(new AcDbLine(pt1, pt2)) {};
	AcGePoint3d startPoint() const;
	bool setStartPoint(const AcGePoint3d&);
	AcGePoint3d endPoint() const;
	bool setEndPoint(const AcGePoint3d&);
	AcGeVector3d normal() const;
	bool setNormal(const AcGeVector3d&);
	double thickness() const;
	bool setThickness(double);
	std::vector<PyDbEntity> getOffsetCurvesGivenPlaneNormal(const AcGeVector3d& nrm, double offsetDist) const;
	AcString toString() const;
	Adesk::UInt16 colorIndex() const;
	void setColorIndex(Adesk::UInt16 color);
	bool isClosed() const;
};

void bind_PyDbLine(pym& m);
