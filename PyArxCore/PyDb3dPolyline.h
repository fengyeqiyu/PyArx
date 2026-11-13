#pragma once
#include "stdafx.h"
#include "PyDbCurve.h"

class PyDb3dPolylineVertex;

class PyDb3dPolyline : public PyDbCurve {
	PyWrapper_H(Db3dPolyline)
public:
	PyDb3dPolyline(AcDb3dPolyline* p) : PyDbCurve(p) {}
	PyDb3dPolyline() : PyDb3dPolyline(new AcDb3dPolyline()) {}
	unsigned int numVerts() const;
	bool setClosed(bool closed);
	double length() const;
	int polyType() const;
	bool setPolyType(int type);
	bool convertToPolyType(int newVal);
	bool makeClosed();
	bool makeOpen();
	bool straighten();
	bool splineFit();
	bool splineFit(int splineType, int splineSegs);
	bool appendVertex(PyDb3dPolylineVertex& vertex);
	uint64_t appendVertexId(PyDb3dPolylineVertex& vertex);
	bool insertVertexAt(const PyDb3dPolylineVertex& indexVertex, PyDb3dPolylineVertex& newVertex);
	uint64_t insertVertexAtById(uint64_t indexVertexId, PyDb3dPolylineVertex& newVertex);
	AcString toString() const;
};

void bind_PyDb3dPolyline(pym& m);
