#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"
#include <dbSubD.h>

class PyDbSubentId;
class PyDbFullSubentPath;

class PyDbSubDMesh : public PyDbEntity {
	PyWrapper_H(DbSubDMesh)
public:
	PyDbSubDMesh() : PyDbSubDMesh(new AcDbSubDMesh()) {}
	PyDbSubDMesh(AcDbSubDMesh* p) : PyDbEntity(p) {}
	int subdLevel() const;
	bool isWatertight() const;
	int numOfFaces() const;
	int numOfVertices() const;
	int numOfEdges() const;
	int numOfSubDividedFaces() const;
	int numOfSubDividedVertices() const;
	AcGePoint3d getVertexAt(int index) const;
	bool setVertexAt(int index, const AcGePoint3d&);
	double computeVolume() const;
	double computeSurfaceArea() const;
	bool subdDivideUp();
	bool subdDivideDown();
	bool subdRefine();
	bool setSphere(double radius, int divAxis, int divHeight, int subDLevel);
	bool setBox(double xLen, double yLen, double zLen, int divX, int divY, int divZ, int subDLevel);
	bool setTorus(double majorRadius, int divSection, int divSweepPath, double sectionRadiusRatio, double sectionRotate, int subDLevel);
	bool setCylinder(double majorRadius, double minorRadius, double height, int divAxis, int divHeight, int divCap, int subDLevel);
	bool setCone(double majorRadius, double minorRadius, double height, int divAxis, int divHeight, int divCap, double radiusRatio, int subDLevel);
	bool setWedge(double xLen, double yLen, double zLen, int divLength, int divWidth, int divHeight, int divSlope, int divCap, int subDLevel);
	bool setPyramid(double radius, double height, int divLength, int divHeight, int divCap, int nSides, double radiusRatio, int subDLevel);
	bool setSubDMesh(const AcGePoint3dArray& vertexArray, const AcArray<Adesk::Int32>& faceArray, int subDLevel);
	bool splitFace(const PyDbSubentId& faceId, const PyDbSubentId& sub0, const AcGePoint3d& p0, const PyDbSubentId& sub1, const AcGePoint3d& p1);
	bool extrudeFaces(const std::vector<PyDbFullSubentPath>& paths, const AcGePoint3dArray& alongPath, double taper);
	bool extrudeConnectedFaces(const std::vector<PyDbFullSubentPath>& paths, const AcGePoint3dArray& alongPath, double taper);
	AcString toString() const;
};

void bind_PyDbSubDMesh(pym& m);
