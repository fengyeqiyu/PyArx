#pragma once
#include "stdafx.h"
#include "PyDbCurve.h"

class PyDbDimStyleTableRecord;

class PyDbLeader : public PyDbCurve {
	PyWrapper_H(DbLeader)
public:
	PyDbLeader(AcDbLeader* p) : PyDbCurve(p) {}
	PyDbLeader() : PyDbLeader(new AcDbLeader()) {}
	int numVertices() const;
	bool appendVertex(const AcGePoint3d& pt);
	AcGePoint3d firstVertex() const;
	AcGePoint3d lastVertex() const;
	AcGePoint3d vertexAt(int i) const;
	bool setVertexAt(int i, const AcGePoint3d& pt);
	void removeLastVertex();
	bool hasArrowHead() const;
	void enableArrowHead();
	void disableArrowHead();
	void setHasArrowHead(bool enable);
	bool hasHookLine() const;
	void setToSplineLeader();
	void setToStraightLeader();
	bool isSplined() const;
	void setSplined(bool b);
	uint64_t annotationObjId() const;
	bool detachAnnotation();
	bool evaluateLeader();
	AcGeVector3d annotationOffset() const;
	bool setAnnotationOffset(const AcGeVector3d& offset);
	int annoType() const;
	double annoHeight() const;
	double annoWidth() const;
	double dimasz() const;
	bool setDimasz(double v);
	double dimgap() const;
	bool setDimgap(double v);
	double dimscale() const;
	bool setDimscale(double v);
	double dimtxt() const;
	bool setDimtxt(double v);
	int dimtad() const;
	bool setDimtad(int v);
	bool dimsah() const;
	bool setDimsah(bool v);
	uint64_t dimensionStyle() const;
	uint64_t dimldrblk() const;
	int dimlwd() const;
	uint64_t dimtxsty() const;
	AcGeVector3d normal() const;
	bool setAnnotationObjId(uint64_t annoId);
	void setDimVars();
	void setDimensionStyle(uint64_t styleId);
	bool setDimldrblk(uint64_t blkId);
	bool setDimlwd(int lw);
	bool setDimstyleData(uint64_t styleId);
	bool setDimtxsty(uint64_t txstyId);
	void setPlane(const AcGePlane& plane);
	PyDbDimStyleTableRecord* getDimstyleData() const;
	AcString toString() const;
};

void bind_PyDbLeader(pym& m);
