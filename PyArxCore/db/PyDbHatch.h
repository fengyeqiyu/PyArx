#pragma once
#include "stdafx.h"
#include "gedblar.h"
#include "PyDbEntity.h"

class PyDbHatch : public PyDbEntity {
	PyWrapper_H(DbHatch)
public:
	PyDbHatch(AcDbHatch* p) : PyDbEntity(p) {}
	PyDbHatch() : PyDbHatch(new AcDbHatch()) {}
	bool associative() const;
	bool setAssociative(bool associative);
	int numLoops() const;
	int loopTypeAt(int loopIndex) const;
	double elevation() const;
	bool setElevation(double);
	AcGeVector3d normal() const;
	bool setNormal(const AcGeVector3d&);
	int hatchObjectType() const;
	bool setHatchObjectType(int);
	bool isGradient() const;
	bool isHatch() const;
	bool isSolidFill() const;
	AcGePoint2d originPoint() const;
	bool setOriginPoint(const AcGePoint2d&);
	int patternType() const;
	AcString patternName() const;
	bool setPattern(int patType, const AcString& patName);
	double patternAngle() const;
	bool setPatternAngle(double);
	double patternSpace() const;
	bool setPatternSpace(double);
	double patternScale() const;
	bool setPatternScale(double);
	bool patternDouble() const;
	bool setPatternDouble(bool);
	int hatchStyle() const;
	bool setHatchStyle(int);
	int numHatchLines() const;
	double area() const;
	bool evaluateHatch(bool underestimate = false);
	bool evaluateHatchAllScales(bool underestimate = false);
	bool lineGenerationEnabled() const;
	bool setLineGenerationEnabled(bool);
	int numPatternDefinitions() const;
	std::tuple<double, double, double, double, double, AcGeDoubleArray> getPatternDefinitionAt(int index) const;
	int gradientType() const;
	AcString gradientName() const;
	double gradientAngle() const;
	bool setGradientAngle(double);
	bool gradientOneColorMode() const;
	bool setGradientOneColorMode(bool);
	float shadeTintValue() const;
	bool setShadeTintValue(float);
	float gradientShift() const;
	bool setGradientShift(float);
	AcString toString() const;
};

void bind_PyDbHatch(pym& m);
