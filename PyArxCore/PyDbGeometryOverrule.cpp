#include "stdafx.h"
#include "PyDbGeometryOverrule.h"
#include "PyDbEntity.h"

bool PyDbGeometryOverrule::isApplicable(const AcRxObject* pOverruledSubject) const {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "isApplicable")) {
        try {
            return py_self.attr("isApplicable")(pybind11::cast(pOverruledSubject)).cast<bool>();
        }
        catch (const pybind11::error_already_set&) { PyErr_Clear(); }
        catch (...) {}
    }
    return true;
}

Acad::ErrorStatus PyDbGeometryOverrule::intersectWith(
    const AcDbEntity* pSubject,
    const AcDbEntity* pEnt,
    AcDb::Intersect intType,
    AcGePoint3dArray& points,
    Adesk::GsMarker thisGsMarker,
    Adesk::GsMarker otherGsMarker)
{
    pybind11::gil_scoped_acquire gil;
    pybind11::handle py_self = pybind11::cast(this);
    if (pybind11::hasattr(py_self, "intersectWith")) {
        try {
            pybind11::list pyPoints;
            for (int i = 0; i < points.length(); i++) {
                pyPoints.append(points[i]);
            }
            auto result = py_self.attr("intersectWith")(
                pybind11::cast(pSubject),
                pybind11::cast(pEnt),
                (int)intType,
                pyPoints,
                thisGsMarker,
                otherGsMarker
            );
            // 从 Python list 取回 points
            points.setLogicalLength(0);
            for (auto item : pyPoints) {
                points.append(item.cast<AcGePoint3d>());
            }
            return Acad::eOk;
        }
        catch (...) {}
    }
    return AcDbGeometryOverrule::intersectWith(pSubject, pEnt, intType, points, thisGsMarker, otherGsMarker);
}

Acad::ErrorStatus PyDbGeometryOverrule::intersectWith(
    const AcDbEntity* pSubject,
    const AcDbEntity* pEnt,
    AcDb::Intersect intType,
    const AcGePlane& projPlane,
    AcGePoint3dArray& points,
    Adesk::GsMarker thisGsMarker,
    Adesk::GsMarker otherGsMarker)
{
    pybind11::gil_scoped_acquire gil;
    pybind11::handle py_self = pybind11::cast(this);
    if (pybind11::hasattr(py_self, "intersectWith")) {
        try {
            pybind11::list pyPoints;
            for (int i = 0; i < points.length(); i++) {
                pyPoints.append(points[i]);
            }
            auto result = py_self.attr("intersectWith")(
                pybind11::cast(pSubject),
                pybind11::cast(pEnt),
                (int)intType,
                projPlane,
                pyPoints,
                thisGsMarker,
                otherGsMarker
            );
            points.setLogicalLength(0);
            for (auto item : pyPoints) {
                points.append(item.cast<AcGePoint3d>());
            }
            return Acad::eOk;
        }
        catch (...) {}
    }
    return AcDbGeometryOverrule::intersectWith(pSubject, pEnt, intType, projPlane, points, thisGsMarker, otherGsMarker);
}

Acad::ErrorStatus PyDbGeometryOverrule::getGeomExtents(const AcDbEntity* pSubject, AcDbExtents& extents) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "getGeomExtents")) {
        try {
            auto result = py_self.attr("getGeomExtents")(pybind11::cast(pSubject));
            auto tuple = result.cast<pybind11::tuple>();
            extents.set(tuple[0].cast<AcGePoint3d>(), tuple[1].cast<AcGePoint3d>());
            return Acad::eOk;
        }
        catch (const pybind11::error_already_set&) { PyErr_Clear(); }
        catch (...) {}
    }
    return AcDbGeometryOverrule::getGeomExtents(pSubject, extents);
}

void bind_PyDbGeometryOverrule(pym& m) {
    pybind11::class_<PyDbGeometryOverrule>(m, "GeometryOverrule",
        "几何 Overrule 基类。Python 子类继承并重写方法以覆盖实体几何行为。\n"
        "重写示例：\n"
        "class MyGeomOverrule(GeometryOverrule):\n"
        "    def isApplicable(self, obj): return True\n"
        "    def intersectWith(self, subject, ent, intType, points, thisMarker=0, otherMarker=0):\n"
        "        return eOk  # points 是 list，可修改后返回")
        .def(pybind11::init<>());
}
