#pragma once
#include "stdafx.h"
#include "PyDbVertex.h"

class PyDb3dPolylineVertex : public PyDbVertex {
	PyWrapper_H(Db3dPolylineVertex)
public:
	PyDb3dPolylineVertex(AcDb3dPolylineVertex* p) : PyDbVertex(p) {}
	PyDb3dPolylineVertex() : PyDb3dPolylineVertex(new AcDb3dPolylineVertex()) {}
	PyDb3dPolylineVertex(const AcGePoint3d& pos) : PyDbVertex(new AcDb3dPolylineVertex(pos)) {}

	AcGePoint3d position() const;
	bool setPosition(const AcGePoint3d&);
	int vertexType() const;
	AcString toString() const;
};

void bind_PyDb3dPolylineVertex(pym& m);
