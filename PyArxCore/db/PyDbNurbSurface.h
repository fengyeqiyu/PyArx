#pragma once
#include "stdafx.h"
#include "dbnurbsurf.h"
#include "PyDbSurface.h"

// AcDbNurbSurface : AcDbSurface  (G:\ObjectArx\2024\inc\dbnurbsurf.h line 25)
// 注：addControlPoint/removeControlPoint 在 CHM 核对清单中无页面，未列入；
//     SDK 头中的 InsertKnotAtU/V、InsertControlPointsAtU/V、RemoveControlPointsAtU/V
//     同样不在本次 CHM 真实方法清单内，按清单范围不绑。
class PyDbNurbSurface : public PyDbSurface {
	PyWrapper_H(DbNurbSurface)
public:
	PyDbNurbSurface(AcDbNurbSurface* p) : PyDbSurface(p) {}
	PyDbNurbSurface() : PyDbSurface(new AcDbNurbSurface()) {}

	// 跳过（虚函数重载，Python 侧无意义）：dragStatus
};

void bind_PyDbNurbSurface(pym& m);
