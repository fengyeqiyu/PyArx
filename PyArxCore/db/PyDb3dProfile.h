#pragma once
#include "stdafx.h"

// AcDb3dProfile (db3dProfile.h:37) — public AcRxObject（非 AcDbObject）。
// 独立绑定：自持 AcDb3dProfile* m_pObj + impObj()，不使用 PyWrapper 宏。
// new 出来的对象由本包装析构 delete。PyDbLoftProfile 继承本类。
class AcDb3dProfile;
class PyDbEntity;
class PyRxObject;

class PyDb3dProfile {
public:
	PyDb3dProfile();
	explicit PyDb3dProfile(AcDb3dProfile* p);          // 接管（析构 delete）
	explicit PyDb3dProfile(PyDbEntity& entity);        // new AcDb3dProfile(entity)
	virtual ~PyDb3dProfile();

	AcDb3dProfile* impObj();
	const AcDb3dProfile* impObj() const;

protected:
	AcDb3dProfile* m_pObj;
};

void bind_PyDb3dProfile(pym& m);
