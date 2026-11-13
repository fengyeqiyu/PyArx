#pragma once
#include "stdafx.h"

// AcDbBlendOptions (dbBlendOptions.h:28) — 无 SDK 基类的纯参数类。
// 独立绑定：自持 AcDbBlendOptions* m_pObj + impObj()，不使用 PyWrapper 宏。
class AcDbBlendOptions;

class PyDbBlendOptions {
public:
	PyDbBlendOptions();
	explicit PyDbBlendOptions(AcDbBlendOptions* p);
	~PyDbBlendOptions();

	AcDbBlendOptions* impObj();
	const AcDbBlendOptions* impObj() const;

private:
	AcDbBlendOptions* m_pObj;
};

void bind_PyDbBlendOptions(pym& m);
