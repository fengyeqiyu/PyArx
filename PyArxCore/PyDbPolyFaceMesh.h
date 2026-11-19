#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"
#include "PyDbSequenceEnd.h"

// AcDbPolyFaceMesh : AcDbEntity  (dbents.h:1020)
// 多边形面网格。appendVertex/appendFaceRecord/openVertex 依赖并行在建的
// PyDbPolyFaceMeshVertex/PyDbFaceRecord/PyDbVertex，列为 P2 依赖暂不绑定。
class PyDbPolyFaceMesh : public PyDbEntity {
public:
	PyWrapper_H(DbPolyFaceMesh)
	PyDbPolyFaceMesh(AcDbPolyFaceMesh* p) : PyDbEntity(p) {}
	PyDbPolyFaceMesh() : PyDbPolyFaceMesh(new AcDbPolyFaceMesh()) {}

	// 打开网格末尾的 SequenceEnd；失败返回空包装（不抛异常）。
	PyDbSequenceEnd openSequenceEnd(int mode) const;
};

void bind_PyDbPolyFaceMesh(pym& m);
