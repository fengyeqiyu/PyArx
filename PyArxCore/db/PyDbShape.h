#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

class PyDbShape : public PyDbEntity {
	PyWrapper_H(DbShape)
public:
	PyDbShape(AcDbShape* p) : PyDbEntity(p) {}
	PyDbShape() : PyDbShape(new AcDbShape()) {}
	PyDbShape(const AcGePoint3d& position, double size, double rotation = 0.0, double widthFactor = 0.0)
		: PyDbEntity(new AcDbShape(position, size, rotation, widthFactor)) {}
	AcGePoint3d position() const;
	bool setPosition(const AcGePoint3d&);
	double size() const;
	bool setSize(double);
	AcString shapeName() const;
	bool setShapeName(const AcString&);
	double rotation() const;
	bool setRotation(double);
	double widthFactor() const;
	bool setWidthFactor(double);
	double oblique() const;
	bool setOblique(double);
	double thickness() const;
	bool setThickness(double);
	AcGeVector3d normal() const;
	bool setNormal(const AcGeVector3d&);
	int shapeNumber() const;
	bool setShapeNumber(int);
	uint64_t styleId() const;
	uint64_t shapeIndex() const;
	bool setStyleId(uint64_t id);
	bool setShapeIndex(uint64_t id);
	AcString toString() const;
};

void bind_PyDbShape(pym& m);
