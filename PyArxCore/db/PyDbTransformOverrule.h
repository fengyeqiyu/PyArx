#pragma once
#include "stdafx.h"
#include "dbentityoverrule.h"

// AcDbTransformOverrule 的 Python 封装（trampoline 模式）
class PyDbTransformOverrule : public AcDbTransformOverrule
{
public:
    using AcDbTransformOverrule::AcDbTransformOverrule;
    bool isApplicable(const AcRxObject* pOverruledSubject) const override;
};

void bind_PyDbTransformOverrule(pym& m);