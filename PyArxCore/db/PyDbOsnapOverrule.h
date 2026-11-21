#pragma once
#include "stdafx.h"
#include "dbentityoverrule.h"

// AcDbOsnapOverrule 的 Python 封装（trampoline 模式）
class PyDbOsnapOverrule : public AcDbOsnapOverrule
{
public:
    using AcDbOsnapOverrule::AcDbOsnapOverrule;
    bool isApplicable(const AcRxObject* pOverruledSubject) const override;
};

void bind_PyDbOsnapOverrule(pym& m);