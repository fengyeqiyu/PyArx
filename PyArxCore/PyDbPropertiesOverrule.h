#pragma once
#include "stdafx.h"
#include "dbentityoverrule.h"

// AcDbPropertiesOverrule 的 Python 封装（trampoline 模式）
class PyDbPropertiesOverrule : public AcDbPropertiesOverrule
{
public:
    using AcDbPropertiesOverrule::AcDbPropertiesOverrule;
    bool isApplicable(const AcRxObject* pOverruledSubject) const override;
};

void bind_PyDbPropertiesOverrule(pym& m);