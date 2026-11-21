#pragma once
#include "stdafx.h"
#include "dbentityoverrule.h"

// AcDbVisibilityOverrule 的 Python 封装（trampoline 模式）
class PyDbVisibilityOverrule : public AcDbVisibilityOverrule
{
public:
    using AcDbVisibilityOverrule::AcDbVisibilityOverrule;
    bool isApplicable(const AcRxObject* pOverruledSubject) const override;
};

void bind_PyDbVisibilityOverrule(pym& m);