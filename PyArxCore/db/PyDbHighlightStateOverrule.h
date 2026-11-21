#pragma once
#include "stdafx.h"
#include "dbentityoverrule.h"

// AcDbHighlightStateOverrule 的 Python 封装（trampoline 模式）
class PyDbHighlightStateOverrule : public AcDbHighlightStateOverrule
{
public:
    using AcDbHighlightStateOverrule::AcDbHighlightStateOverrule;
    bool isApplicable(const AcRxObject* pOverruledSubject) const override;
};

void bind_PyDbHighlightStateOverrule(pym& m);