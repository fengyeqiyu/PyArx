#pragma once
#include "stdafx.h"
#include "dbsweptsurf.h"
#include "PyDbSurface.h"

// AcDbSweptSurface : AcDbSurface  (G:\ObjectArx\2024\inc\dbsweptsurf.h)
class PyDbSweptSurface : public PyDbSurface {
	PyWrapper_H(DbSweptSurface)
public:
	PyDbSweptSurface(AcDbSweptSurface* p) : PyDbSurface(p) {}

	double getPathLength() const;
	bool isDependent() const;

	// P2 依赖（待 AcDbSweepOptions / AcDb3dProfile 包装完成后补绑）：
	//   createSweptSurface(AcDbEntity* pSweepEnt, AcDbEntity* pPathEnt, AcDbSweepOptions&)
	//   getSweepOptions(AcDbSweepOptions&) const / setSweepOptions(const AcDbSweepOptions&)
	// 跳过（内部持久化重载）：dwg/dxf In/OutFields、subGetClassID
	// 疑误提取（CHM 无页面）：pack
};

void bind_PyDbSweptSurface(pym& m);
