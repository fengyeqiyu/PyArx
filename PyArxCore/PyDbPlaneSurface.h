#pragma once
#include "stdafx.h"
#include "dbplanesurf.h"
#include "PyDbSurface.h"

class PyDbRegion;

// AcDbPlaneSurface : AcDbSurface  (G:\ObjectArx\2024\inc\dbplanesurf.h line 26)
class PyDbPlaneSurface : public PyDbSurface {
	PyWrapper_H(DbPlaneSurface)
public:
	PyDbPlaneSurface(AcDbPlaneSurface* p) : PyDbSurface(p) {}

	bool createFromRegion(PyDbRegion& region);
	bool isDependent() const;

	// 跳过（内部持久化重载，AcDbDwgFiler/AcDbDxfFiler/CLSID 未包装，Python 侧无意义）：
	//   dwgInFields / dwgOutFields / dxfInFields / dxfOutFields / subGetClassID
	// 疑误提取（CHM 无页面）：pack
};

void bind_PyDbPlaneSurface(pym& m);
