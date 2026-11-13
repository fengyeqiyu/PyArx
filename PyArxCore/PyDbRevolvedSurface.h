#pragma once
#include "stdafx.h"
#include "dbrevolvedsurf.h"
#include "PyDbSurface.h"

// AcDbRevolvedSurface : AcDbSurface  (G:\ObjectArx\2024\inc\dbrevolvedsurf.h)
class PyDbRevolvedSurface : public PyDbSurface {
	PyWrapper_H(DbRevolvedSurface)
public:
	PyDbRevolvedSurface(AcDbRevolvedSurface* p) : PyDbSurface(p) {}

	AcGePoint3d getAxisPnt() const;
	AcGeVector3d getAxisVec() const;
	double getRevolveAngle() const;
	double getStartAngle() const;
	bool setAxisPnt(const AcGePoint3d& pnt);
	bool setAxisVec(const AcGeVector3d& vec);
	bool setRevolveAngle(double ang);

	// P2 依赖（待 AcDbRevolveOptions / AcDb3dProfile 包装完成后补绑）：
	//   createRevolvedSurface(AcDbEntity*, const AcGePoint3d&, const AcGeVector3d&,
	//                         double revAngle, double startAngle, AcDbRevolveOptions&)
	//   setRevolve(const AcGePoint3d&, const AcGeVector3d&, double, const AcDbRevolveOptions&)
	//   getRevolveOptions(AcDbRevolveOptions&) const / setRevolveOptions(const AcDbRevolveOptions&)
	// 跳过（内部持久化重载）：dwg/dxf In/OutFields、subGetClassID
};

void bind_PyDbRevolvedSurface(pym& m);
