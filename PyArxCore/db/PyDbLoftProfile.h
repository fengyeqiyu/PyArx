#pragma once
#include "stdafx.h"
#include "PyDb3dProfile.h"

// AcDbLoftProfile (db3dProfile.h:320) — public AcDb3dProfile。
// 继承 PyDb3dProfile（自持指针体系），绑本类新增的 continuity/magnitude 具体方法。
class AcDbLoftProfile;
class PyDbEntity;
class AcGePoint3d;

class PyDbLoftProfile : public PyDb3dProfile {
public:
	PyDbLoftProfile();
	explicit PyDbLoftProfile(AcDbLoftProfile* p);       // 接管
	explicit PyDbLoftProfile(PyDbEntity& entity);       // new AcDbLoftProfile(entity)
	explicit PyDbLoftProfile(const AcGePoint3d& pnt);  // new AcDbLoftProfile(point)
};

void bind_PyDbLoftProfile(pym& m);
