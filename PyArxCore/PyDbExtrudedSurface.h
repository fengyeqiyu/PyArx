#pragma once
#include "stdafx.h"
#include "dbextrudedsurf.h"
#include "PyDbSurface.h"

// AcDbExtrudedSurface : AcDbSurface  (G:\ObjectArx\2024\inc\dbextrudedsurf.h line 26)
class PyDbExtrudedSurface : public PyDbSurface {
	PyWrapper_H(DbExtrudedSurface)
public:
	PyDbExtrudedSurface(AcDbExtrudedSurface* p) : PyDbSurface(p) {}

	double getHeight() const;
	AcGeVector3d getSweepVec() const;
	bool setHeight(double height);
	bool setSweepVec(const AcGeVector3d& sweepVec);
	bool isDependent() const;

	// P2 依赖（待 AcDbSweepOptions 包装完成后补绑）：
	//   createExtrudedSurface(AcDbEntity*, const AcGeVector3d&, AcDbSweepOptions&)
	//   setExtrude(const AcGeVector3d&, const AcDbSweepOptions&)
	//   getSweepOptions(AcDbSweepOptions&) const / setSweepOptions(const AcDbSweepOptions&)
	// 跳过（内部持久化重载）：dwg/dxf In/OutFields、subGetClassID
	// 疑误提取（CHM 无页面）：pack
};

void bind_PyDbExtrudedSurface(pym& m);
