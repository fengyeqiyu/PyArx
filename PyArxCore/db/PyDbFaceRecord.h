#pragma once
#include "stdafx.h"
#include "PyDbVertex.h"

class PyDbFaceRecord : public PyDbVertex {
	PyWrapper_H(DbFaceRecord)
public:
	PyDbFaceRecord(AcDbFaceRecord* p) : PyDbVertex(p) {}
	PyDbFaceRecord() : PyDbFaceRecord(new AcDbFaceRecord()) {}
	PyDbFaceRecord(int v0, int v1, int v2, int v3)
		: PyDbVertex(new AcDbFaceRecord((Adesk::Int16)v0, (Adesk::Int16)v1, (Adesk::Int16)v2, (Adesk::Int16)v3)) {}

	// 返回面索引 faceIdx(0-3) 处的顶点索引；失败返回 0
	int getVertexAt(int faceIdx) const;
	bool setVertexAt(int faceIdx, int vtxIdx);
	// 返回指定边(faceIdx 0-3)是否可见；失败返回 false
	bool isEdgeVisibleAt(int faceIndex) const;
	bool makeEdgeVisibleAt(int faceIndex);
	bool makeEdgeInvisibleAt(int faceIndex);
	AcString toString() const;
};

void bind_PyDbFaceRecord(pym& m);
