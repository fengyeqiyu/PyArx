#pragma once
#include "stdafx.h"

// AcDbSweepOptions (dbSweepOptions.h:30) — 无 SDK 基类的纯参数类。
// 独立绑定：自持 AcDbSweepOptions* m_pObj + impObj()，不使用 PyWrapper 宏。
class AcDbSweepOptions;

class PyDbSweepOptions {
public:
	PyDbSweepOptions();
	explicit PyDbSweepOptions(AcDbSweepOptions* p);
	~PyDbSweepOptions();

	AcDbSweepOptions* impObj();
	const AcDbSweepOptions* impObj() const;

private:
	AcDbSweepOptions* m_pObj;
};

void bind_PyDbSweepOptions(pym& m);
