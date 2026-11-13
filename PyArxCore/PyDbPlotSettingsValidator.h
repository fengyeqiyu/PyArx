#pragma once
#include "stdafx.h"
#include "dbplotsetval.h"

// AcDbPlotSettingsValidator has NO base class (dbplotsetval.h:34, pure abstract).
// 独立绑定：自身持有 m_pObj，不继承 PyRxObject（非 AcRxObject 派生）。
// 实例由 acdbHostApplicationServices()->plotSettingsValidator() 获取，
// 归 AutoCAD 所有，包装不 delete。
class PyDbPlotSettingsValidator {
public:
	PyDbPlotSettingsValidator(AcDbPlotSettingsValidator* p = nullptr) : m_pObj(p) {}
	AcDbPlotSettingsValidator* impObj() { return m_pObj; }
	AcDbPlotSettingsValidator* impObj() const { return m_pObj; }

	static PyDbPlotSettingsValidator get();
	bool isNull() const { return m_pObj == nullptr; }

private:
	AcDbPlotSettingsValidator* m_pObj;
};

void bind_PyDbPlotSettingsValidator(pym& m);
