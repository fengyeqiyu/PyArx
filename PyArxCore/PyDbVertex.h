#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

// AcDbVertex 是所有 Vertex 派生类的 SDK 抽象基类（dbents.h:460）。
// 自身无独立方法，仅作为 PyDb2dVertex / PyDb3dPolylineVertex 等的 Python 基类。
class PyDbVertex : public PyDbEntity {
	PyWrapper_H(DbVertex)
public:
	PyDbVertex(AcDbVertex* p) : PyDbEntity(p) {}
	PyDbVertex() : PyDbVertex(new AcDbVertex()) {}
};

void bind_PyDbVertex(pym& m);
