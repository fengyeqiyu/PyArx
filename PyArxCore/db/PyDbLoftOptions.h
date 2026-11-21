#pragma once
#include "stdafx.h"

// AcDbLoftOptions (dbLoftOptions.h:31) — 无 SDK 基类的纯参数类。
// 独立绑定：自持 AcDbLoftOptions* m_pObj + impObj()，不使用 PyWrapper 宏。
// new 出来的对象由本包装析构 delete。
class AcDbLoftOptions;

class PyDbLoftOptions {
public:
	PyDbLoftOptions();
	explicit PyDbLoftOptions(AcDbLoftOptions* p);
	~PyDbLoftOptions();

	AcDbLoftOptions* impObj();
	const AcDbLoftOptions* impObj() const;

private:
	AcDbLoftOptions* m_pObj;
};

void bind_PyDbLoftOptions(pym& m);
