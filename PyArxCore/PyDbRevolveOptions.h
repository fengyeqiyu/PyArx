#pragma once
#include "stdafx.h"

// AcDbRevolveOptions (dbRevolveOptions.h:29) — 无 SDK 基类的纯参数类。
// 独立绑定：自持 AcDbRevolveOptions* m_pObj + impObj()，不使用 PyWrapper 宏。
class AcDbRevolveOptions;

class PyDbRevolveOptions {
public:
	PyDbRevolveOptions();
	explicit PyDbRevolveOptions(AcDbRevolveOptions* p);
	~PyDbRevolveOptions();

	AcDbRevolveOptions* impObj();
	const AcDbRevolveOptions* impObj() const;

private:
	AcDbRevolveOptions* m_pObj;
};

void bind_PyDbRevolveOptions(pym& m);
