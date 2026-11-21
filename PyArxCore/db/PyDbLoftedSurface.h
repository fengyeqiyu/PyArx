#pragma once
#include "stdafx.h"
#include "dbloftedsurf.h"
#include "PyDbSurface.h"

// AcDbLoftedSurface : AcDbSurface  (G:\ObjectArx\2024\inc\dbloftedsurf.h)
class PyDbLoftedSurface : public PyDbSurface {
	PyWrapper_H(DbLoftedSurface)
public:
	PyDbLoftedSurface(AcDbLoftedSurface* p) : PyDbSurface(p) {}
	PyDbLoftedSurface() : PyDbLoftedSurface(new AcDbLoftedSurface()) {}

	int numCrossSections() const;
	int numGuideCurves() const;

	// P2 依赖（待 AcDbLoftOptions / AcDbLoftProfile 包装完成后补绑）：
	//   createLoftedSurface(AcArray<AcDbEntity*>& crossSectionCurves,
	//                       AcArray<AcDbEntity*>& guideCurves,
	//                       AcDbEntity* pPathCurve, AcDbLoftOptions&)
	//   getCrossSections(AcArray<AcDbLoftProfile*>&) const
	//   getLoftOptions(AcDbLoftOptions&) const / setLoftOptions(const AcDbLoftOptions&)
};

void bind_PyDbLoftedSurface(pym& m);
