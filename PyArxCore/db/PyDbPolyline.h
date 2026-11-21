#pragma once
#include "stdafx.h"
#include "PyDbCurve.h"

class PyDb2dPolyline;

class PyDbPolyline : public PyDbCurve {
	PyWrapper_H(DbPolyline)
public:
	PyDbPolyline(AcDbPolyline* p) : PyDbCurve(p) {}
	PyDbPolyline() : PyDbPolyline(new AcDbPolyline()) {}
	unsigned int numVerts() const;
	AcGePoint2d getPointAt(unsigned int index) const;
	bool setPointAt(unsigned int index, const AcGePoint2d&);
	void setClosed(bool closed);

	double getBulgeAt(unsigned int index) const;
	bool setBulgeAt(unsigned int index, double bulge);
	py::tuple getWidthsAt(unsigned int index) const;
	bool setWidthsAt(unsigned int index, double startWidth, double endWidth);
	bool addVertexAt(unsigned int index, const AcGePoint2d& pt, double bulge = 0.0,
		double startWidth = -1.0, double endWidth = -1.0);
	bool removeVertexAt(unsigned int index);

	double elevation() const;
	void setElevation(double);
	double thickness() const;
	bool setThickness(double);
	double constantWidth() const;
	bool setConstantWidth(double);
	AcGeVector3d normal() const;
	bool setNormal(const AcGeVector3d&);
	void setPlinegen(bool on);
	bool hasBulges() const;
	bool hasWidth() const;
	bool hasPlinegen() const;
	bool isOnlyLines() const;
	bool hasVertexIdentifiers() const;
	int segType(unsigned int index) const;
	AcGeLineSeg2d getLineSegAt(unsigned int index) const;
	AcGeCircArc2d getArcSegAt(unsigned int index) const;
	AcGeMatrix3d getEcs() const;
	std::tuple<bool, double> onSegAt(unsigned int index, const AcGePoint2d& pt2d) const;
	void reset(bool reuse, unsigned int numVerts);
	bool makeClosedIfStartAndEndVertexCoincide(double distTol);
	bool convertFrom(const PyDbEntity& src, bool transferId = true);
	PyDb2dPolyline convertTo(bool transferId = true) const;
	AcString toString() const;
	// 静态方法：合并当前空间里相连的 LINE/ARC/LWPOLYLINE 成新的 LWPOLYLINE
	// 返回合并出的 LWPOLYLINE 数量
	static int join();
};

void bind_PyDbPolyline(pym& m);
