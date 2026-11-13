#pragma once
#include "stdafx.h"
#include "PyDbCurve.h"

class PyDb2dVertex;

class PyDb2dPolyline : public PyDbCurve {
	PyWrapper_H(Db2dPolyline)
public:
	PyDb2dPolyline(AcDb2dPolyline* p) : PyDbCurve(p) {}
	PyDb2dPolyline() : PyDb2dPolyline(new AcDb2dPolyline()) {}
	unsigned int numVerts() const;
	bool isClosed() const;
	bool setClosed(bool closed);
	double elevation() const;
	bool setElevation(double);

	int polyType() const;
	bool setPolyType(int type);
	bool convertToPolyType(int newVal);
	bool makeClosed();
	bool makeOpen();
	double constantWidth() const;
	bool setConstantWidth(double w);
	double length() const;
	double defaultStartWidth() const;
	bool setDefaultStartWidth(double w);
	double defaultEndWidth() const;
	bool setDefaultEndWidth(double w);
	double thickness() const;
	bool setThickness(double t);
	AcGeVector3d normal() const;
	bool setNormal(const AcGeVector3d& n);
	bool isLinetypeGenerationOn() const;
	bool setLinetypeGenerationOn();
	bool setLinetypeGenerationOff();
	bool setLinetypeGenerationOn(bool on);
	bool straighten();
	bool splineFit();
	bool splineFit(int splineType, int splineSegs);
	bool curveFit();
	bool makeClosedIfStartAndEndVertexCoincide(double distTol);
	bool appendVertex(PyDb2dVertex& vertex);
	uint64_t appendVertexId(PyDb2dVertex& vertex);
	bool insertVertexAt(const PyDb2dVertex& indexVertex, PyDb2dVertex& newVertex);
	uint64_t insertVertexAtById(uint64_t indexVertexId, PyDb2dVertex& newVertex);
	AcString toString() const;
};

void bind_PyDb2dPolyline(pym& m);
