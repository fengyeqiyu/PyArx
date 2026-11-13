#pragma once
#include "stdafx.h"
#include "dbobjectoverrule.h"

// AcDbObjectOverrule 的 Python 封装（trampoline 模式）
// Python 子类继承并重写方法以覆盖对象的默认行为
class PyDbObjectOverrule : public AcDbObjectOverrule
{
public:
    using AcDbObjectOverrule::AcDbObjectOverrule;

    // trampoline 虚函数重写
    Acad::ErrorStatus open(AcDbObject* pSubject, AcDb::OpenMode mode) override;
    Acad::ErrorStatus close(AcDbObject* pSubject) override;
    Acad::ErrorStatus cancel(AcDbObject* pSubject) override;
    Acad::ErrorStatus erase(AcDbObject* pSubject, Adesk::Boolean erasing) override;
    bool isApplicable(const AcRxObject* pOverruledSubject) const override;
};

void bind_PyDbObjectOverrule(pym& m);
