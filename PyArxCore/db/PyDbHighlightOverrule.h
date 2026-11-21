#pragma once
#include "stdafx.h"
#include "dbentityoverrule.h"

// AcDbHighlightOverrule 的 Python 封装（trampoline 模式）
class PyDbHighlightOverrule : public AcDbHighlightOverrule
{
public:
    using AcDbHighlightOverrule::AcDbHighlightOverrule;
    bool isApplicable(const AcRxObject* pOverruledSubject) const override;
};

void bind_PyDbHighlightOverrule(pym& m);