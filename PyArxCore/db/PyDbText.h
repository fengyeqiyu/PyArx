#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

class PyDbText : public PyDbEntity {
	PyWrapper_H(DbText)
public:
	PyDbText(AcDbText* p) : PyDbEntity(p) {}
	PyDbText() : PyDbText(new AcDbText()) {}
	PyDbText(const AcGePoint3d& position, const AcString& text, double height = 0.0, double rotation = 0.0)
		: PyDbEntity(new AcDbText(position, text.kwszPtr(), AcDbObjectId::kNull, height, rotation)) {}
	AcGePoint3d position() const;
	bool setPosition(const AcGePoint3d&);
	AcGePoint3d alignmentPoint() const;
	bool setAlignmentPoint(const AcGePoint3d&);
	bool isDefaultAlignment() const;
	AcGeVector3d normal() const;
	bool setNormal(const AcGeVector3d&);
	double thickness() const;
	bool setThickness(double);
	double oblique() const;
	bool setOblique(double);
	double height() const;
	bool setHeight(double);
	double rotation() const;
	bool setRotation(double);
	double widthFactor() const;
	bool setWidthFactor(double);
	AcString textString() const;
	bool setTextString(const AcString&);
	bool isMirroredInX() const;
	bool mirrorInX(bool);
	bool isMirroredInY() const;
	bool mirrorInY(bool);
	int horizontalMode() const;
	bool setHorizontalMode(int);
	int verticalMode() const;
	bool setVerticalMode(int);
	int justification() const;
	bool setJustification(int);
	bool hitTest(const AcGePoint3d& pt) const;
	int correctSpelling();
	uint64_t textStyle() const;
	bool setTextStyle(uint64_t);
	bool adjustAlignment();
	std::vector<AcGePoint3d> getBoundingPoints() const;
	bool convertFieldToText();
	py::tuple getPlane() const;
	bool isPlanar() const;
	bool removeField(const AcString& propName);
	bool removeField(uint64_t fieldId);
	AcString toString() const;
};

void bind_PyDbText(pym& m);
