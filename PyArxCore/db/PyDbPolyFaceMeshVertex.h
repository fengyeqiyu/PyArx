#pragma once
#include "stdafx.h"
#include "PyDbVertex.h"

class PyDbPolyFaceMeshVertex : public PyDbVertex {
	PyWrapper_H(DbPolyFaceMeshVertex)
public:
	PyDbPolyFaceMeshVertex(AcDbPolyFaceMeshVertex* p) : PyDbVertex(p) {}
	PyDbPolyFaceMeshVertex() : PyDbPolyFaceMeshVertex(new AcDbPolyFaceMeshVertex()) {}
	PyDbPolyFaceMeshVertex(const AcGePoint3d& pos) : PyDbVertex(new AcDbPolyFaceMeshVertex(pos)) {}

	AcGePoint3d position() const;
	bool setPosition(const AcGePoint3d&);
	AcString toString() const;
};

void bind_PyDbPolyFaceMeshVertex(pym& m);
