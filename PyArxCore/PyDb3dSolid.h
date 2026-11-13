#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

class PyDbRegion;
class PyDbCurve;

class PyDb3dSolid : public PyDbEntity {
	PyWrapper_H(Db3dSolid)
public:
	PyDb3dSolid(AcDb3dSolid* p) : PyDbEntity(p) {}
	PyDb3dSolid() : PyDb3dSolid(new AcDb3dSolid()) {}
	bool isNull() const;
	bool createBox(double xLen, double yLen, double zLen);
	bool createSphere(double radius);
	bool createTorus(double majorRadius, double minorRadius);
	bool createWedge(double xLen, double yLen, double zLen);
	bool createFrustum(double height, double xRadius, double yRadius, double topXRadius);
	bool createPyramid(double height, int sides, double radius, double topRadius = 0.0);
	bool booleanOper(int operation, PyDb3dSolid& other);
	bool cleanBody();
	bool offsetBody(double offsetDistance);
	unsigned int numChanges() const;
	bool recordHistory() const;
	bool setRecordHistory(bool);
	bool showHistory() const;
	bool setShowHistory(bool);
	double getArea() const;
	double volume() const;
	bool createFrom(PyDbEntity& src);
	bool extrudeAlongPath(PyDbRegion& region, PyDbCurve& path, double taperAngle = 0.0);
	AcString toString() const;
};

void bind_PyDb3dSolid(pym& m);
