#pragma once
#include "stdafx.h"
#include "dbsurf.h"
#include "PyDbEntity.h"
#include <optional>

// AcDbSurface : AcDbEntity  (G:\ObjectArx\2024\inc\dbsurf.h line 43)
// 所有曲面的抽象基类；PyDbEntity 已绑的通用实体能力此处不重复。
class PyDbSurface : public PyDbEntity {
	PyWrapper_H(DbSurface)
public:
	PyDbSurface(AcDbSurface* p) : PyDbEntity(p) {}

	// static Acad::ErrorStatus createFrom(const AcDbEntity* pFromEntity, AcDbSurface*& pNewSurface)
	// 返回新曲面包装（包装接管指针所有权）；失败返回 nullopt。
	static std::optional<PyDbSurface> createFrom(PyDbEntity& srcEntity);

	// P2 依赖（待 AcDb3dProfile / AcDbSweepOptions / AcDbRevolveOptions /
	// AcDbLoftOptions 等包装完成后补绑）：
	//   createExtrudedSurface / createRevolvedSurface / createSweptSurface /
	//   createLoftedSurface / createNetworkSurface / createPatchSurface /
	//   createBlendSurface / createFilletSurface / createOffsetSurface /
	//   createExtendSurface / trimSurface
};

void bind_PyDbSurface(pym& m);
