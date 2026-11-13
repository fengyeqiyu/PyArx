#pragma once
#include "stdafx.h"
#include "PyDbVertex.h"

class PyDbPolygonMeshVertex : public PyDbVertex {
	PyWrapper_H(DbPolygonMeshVertex)
public:
	PyDbPolygonMeshVertex(AcDbPolygonMeshVertex* p) : PyDbVertex(p) {}
	PyDbPolygonMeshVertex() : PyDbPolygonMeshVertex(new AcDbPolygonMeshVertex()) {}
	PyDbPolygonMeshVertex(const AcGePoint3d& pos) : PyDbVertex(new AcDbPolygonMeshVertex(pos)) {}

	AcGePoint3d position() const;
	bool setPosition(const AcGePoint3d&);
	int vertexType() const;
	AcString toString() const;
};

void bind_PyDbPolygonMeshVertex(pym& m);
