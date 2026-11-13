#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"
#include "PyDbSequenceEnd.h"

// AcDbPolygonMesh : AcDbEntity  (dbents.h:1048)
// 多边形网格。appendVertex/openVertex 依赖并行在建的 PyDbPolygonMeshVertex，列为 P2 依赖暂不绑定。
class PyDbPolygonMesh : public PyDbEntity {
public:
	PyWrapper_H(DbPolygonMesh)
	PyDbPolygonMesh(AcDbPolygonMesh* p) : PyDbEntity(p) {}

	// 打开网格末尾的 SequenceEnd；失败返回空包装（不抛异常）。
	PyDbSequenceEnd openSequenceEnd(int mode) const;
};

void bind_PyDbPolygonMesh(pym& m);
