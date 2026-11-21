#pragma once
#include "stdafx.h"
#include "dbentityoverrule.h"

// AcDbSubentityOverrule 的 Python 封装（trampoline 模式）
class PyDbSubentityOverrule : public AcDbSubentityOverrule
{
public:
    using AcDbSubentityOverrule::AcDbSubentityOverrule;
    bool isApplicable(const AcRxObject* pOverruledSubject) const override;
};

void bind_PyDbSubentityOverrule(pym& m);