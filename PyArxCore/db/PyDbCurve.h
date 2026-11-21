#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

// Base wrapper for AcDbCurve (pointer ctor only). Generic curve capability is
// bound here once; derived curves (Line/Arc/Circle/Polyline/Spline/Ellipse...)
// inherit it and must not rebind these methods.
class PyDbCurve : public PyDbEntity {
	PyWrapper_H(DbCurve)
public:
	PyDbCurve(AcDbCurve* p) : PyDbEntity(p) {}

	bool isClosed() const;
	bool isPeriodic() const;

	double getStartParam() const;
	double getEndParam() const;
	AcGePoint3d getStartPoint() const;
	AcGePoint3d getEndPoint() const;

	AcGePoint3d getPointAtParam(double param) const;
	double getParamAtPoint(const AcGePoint3d& pt) const;
	double getDistAtParam(double param) const;
	double getParamAtDist(double dist) const;
	double getDistAtPoint(const AcGePoint3d& pt) const;
	AcGePoint3d getPointAtDist(double dist) const;

	AcGeVector3d getFirstDeriv(double param) const;
	AcGeVector3d getFirstDeriv(const AcGePoint3d& pt) const;
	AcGeVector3d getSecondDeriv(double param) const;
	AcGeVector3d getSecondDeriv(const AcGePoint3d& pt) const;

	AcGePoint3d getClosestPointTo(const AcGePoint3d& pt, bool extend = false) const;
	// 方向投影版：direction 为投影方向(入参)，返回曲线上最近点
	AcGePoint3d getClosestPointTo(const AcGePoint3d& givenPnt, const AcGeVector3d& direction, bool extend = false) const;

	bool extendToPoint(bool extendStart, const AcGePoint3d& toPoint);
	std::vector<PyDbEntity> getSplitCurves(const std::vector<AcGePoint3d>& points) const;
	std::vector<PyDbEntity> getOffsetCurvesGivenPlaneNormal(const AcGeVector3d& normal, double offsetDist) const;

	double getArea() const;
	bool reverseCurve();
	bool extend(double newParam);
};

void bind_PyDbCurve(pym& m);
