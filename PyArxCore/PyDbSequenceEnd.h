#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

// AcDbSequenceEnd : AcDbEntity  (dbents.h:452)
// SDK 仅提供构造/析构，无业务方法；作为 PolygonMesh/PolyFaceMesh 的序列尾标记被 openSequenceEnd() 打开后返回。
class PyDbSequenceEnd : public PyDbEntity {
public:
	PyWrapper_H(DbSequenceEnd)
	PyDbSequenceEnd(AcDbSequenceEnd* p) : PyDbEntity(p) {}
};

void bind_PyDbSequenceEnd(pym& m);
