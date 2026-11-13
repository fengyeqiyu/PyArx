#pragma once
#include "stdafx.h"
#include "PyDbVertex.h"

class PyDb2dVertex : public PyDbVertex {
	PyWrapper_H(Db2dVertex)
public:
	PyDb2dVertex(AcDb2dVertex* p) : PyDbVertex(p) {}
	PyDb2dVertex() : PyDb2dVertex(new AcDb2dVertex()) {}
	PyDb2dVertex(const AcGePoint3d& pos) : PyDbVertex(new AcDb2dVertex(pos)) {}

	AcGePoint3d position() const;
	bool setPosition(const AcGePoint3d&);
	double startWidth() const;
	bool setStartWidth(double);
	double endWidth() const;
	bool setEndWidth(double);
	double bulge() const;
	bool setBulge(double);
	bool isTangentUsed() const;
	bool useTangent();
	bool ignoreTangent();
	bool setTangentUsed(bool);
	double tangent() const;
	bool setTangent(double);
	int vertexType() const;
	AcString toString() const;
};

void bind_PyDb2dVertex(pym& m);
