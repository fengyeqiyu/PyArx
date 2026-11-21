#pragma once
#include "stdafx.h"
#include "dbentityoverrule.h"

// AcDbGripOverrule 的 Python 封装（trampoline 模式，参考 PyRx）
class PyDbGripOverrule : public AcDbGripOverrule
{
public:
    using AcDbGripOverrule::AcDbGripOverrule;

    // 虚函数重写（trampoline）
    bool isApplicable(const AcRxObject* pOverruledSubject) const override;
    Acad::ErrorStatus getGripPoints(const AcDbEntity* pSubject, AcGePoint3dArray& gripPoints, AcDbIntArray& osnapModes, AcDbIntArray& geomIds) override;
    Acad::ErrorStatus moveGripPointsAt(AcDbEntity* pSubject, const AcDbIntArray& indices, const AcGeVector3d& offset) override;
    Acad::ErrorStatus getStretchPoints(const AcDbEntity* pSubject, AcGePoint3dArray& stretchPoints) override;
    Acad::ErrorStatus moveStretchPointsAt(AcDbEntity* pSubject, const AcDbIntArray& indices, const AcGeVector3d& offset) override;
    void gripStatus(AcDbEntity* pSubject, const AcDb::GripStat status) override;

    // 回调开关（Python 子类可设为 False 禁用某回调）
    mutable bool reg_isApplicable = true;
    mutable bool reg_getGripPoints = true;
    mutable bool reg_moveGripPointsAt = true;
    mutable bool reg_getStretchPoints = true;
    mutable bool reg_moveStretchPointsAt = true;
    mutable bool reg_gripStatus = true;
};

void bind_PyDbGripOverrule(pym& m);
