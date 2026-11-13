#pragma once
#include "stdafx.h"
#include "dbentityoverrule.h"

// AcDbGeometryOverrule 的 Python 封装（trampoline 模式）
class PyDbGeometryOverrule : public AcDbGeometryOverrule
{
public:
    using AcDbGeometryOverrule::AcDbGeometryOverrule;

    bool isApplicable(const AcRxObject* pOverruledSubject) const override;

    Acad::ErrorStatus intersectWith(
        const AcDbEntity* pSubject,
        const AcDbEntity* pEnt,
        AcDb::Intersect intType,
        AcGePoint3dArray& points,
        Adesk::GsMarker thisGsMarker = 0,
        Adesk::GsMarker otherGsMarker = 0) override;

    Acad::ErrorStatus intersectWith(
        const AcDbEntity* pSubject,
        const AcDbEntity* pEnt,
        AcDb::Intersect intType,
        const AcGePlane& projPlane,
        AcGePoint3dArray& points,
        Adesk::GsMarker thisGsMarker = 0,
        Adesk::GsMarker otherGsMarker = 0) override;

    Acad::ErrorStatus getGeomExtents(const AcDbEntity* pSubject, AcDbExtents& extents) override;
};

void bind_PyDbGeometryOverrule(pym& m);
