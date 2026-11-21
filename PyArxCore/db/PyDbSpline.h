#pragma once
#include "stdafx.h"
#include "PyDbCurve.h"

class PyDbSpline : public PyDbCurve {
	PyWrapper_H(DbSpline)
public:
	PyDbSpline(AcDbSpline* p) : PyDbCurve(p) {}
	PyDbSpline() : PyDbSpline(new AcDbSpline()) {}
	PyDbSpline(const AcGePoint3dArray& fitPoints, const AcGeVector3d& startTangent, const AcGeVector3d& endTangent, int knotParam, int degree = 3, double fitTolerance = 0.0);
	PyDbSpline(int degree, bool rational, bool closed, bool periodic, const AcGePoint3dArray& controlPoints, const AcGeDoubleArray& knots, const AcGeDoubleArray& weights, double controlPtTol = 0.0, double knotTol = AcGeKnotVector::globalKnotTolerance);
	int numControlPoints() const;
	int degree() const;
	bool isNull() const;
	bool isRational() const;
	bool elevateDegree(int newDegree);
	AcGePoint3d getControlPointAt(int index) const;
	bool setControlPointAt(int index, const AcGePoint3d& point);
	int numFitPoints() const;
	AcGePoint3d getFitPointAt(int index) const;
	bool setFitPointAt(int index, const AcGePoint3d& point);
	bool insertFitPointAt(int index, const AcGePoint3d& point);
	bool removeFitPointAt(int index);
	double fitTolerance() const;
	bool setFitTol(double tol);
	double weightAt(int index) const;
	bool setWeightAt(int index, double weight);
	bool insertKnot(double param);
	py::tuple getFitTangents() const;
	bool setFitTangents(const AcGeVector3d& startTangent, const AcGeVector3d& endTangent);
	bool hasFitData() const;
	bool purgeFitData();
	bool updateFitData();
	std::tuple<AcGePoint3dArray, bool, AcGeVector3d, AcGeVector3d, int, int, double> getFitData() const;
	std::tuple<int, bool, bool, bool, AcGePoint3dArray, AcGeDoubleArray, AcGeDoubleArray, double, double> getNurbsData() const;
	bool setFitData(const AcGePoint3dArray& fitPoints, const AcGeVector3d& startTangent, const AcGeVector3d& endTangent, int knotParam, int degree = 3, double fitTolerance = 0.0);
	bool setNurbsData(int degree, bool rational, bool closed, bool periodic, const AcGePoint3dArray& controlPoints, const AcGeDoubleArray& knots, const AcGeDoubleArray& weights, double controlPtTol = 0.0, double knotTol = 0.0);
	PyDbCurve toPolyline(int precision = 10, unsigned int numOfVerticesThreshold = kPlineVerticesThrehold) const;
	PyDbCurve toPolylineAs(bool bConvertAsArcs, bool bToLWPolyline, int precision = 10) const;
	bool insertControlPointAt(double knotParam, const AcGePoint3d& ctrlPt, double weight = 1.0);
	bool removeControlPointAt(int index);
	AcString toString() const;
};

void bind_PyDbSpline(pym& m);
