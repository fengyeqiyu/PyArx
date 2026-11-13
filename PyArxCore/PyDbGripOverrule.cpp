#include "stdafx.h"
#include "PyDbGripOverrule.h"
#include "PyDbEntity.h"

// 辅助：AcGePoint3dArray → pybind11::list
static pybind11::list point3dArrayToList(const AcGePoint3dArray& arr) {
    pybind11::list lst;
    for (int i = 0; i < arr.length(); i++) {
        lst.append(arr[i]);
    }
    return lst;
}

// 辅助：pybind11::list → AcGePoint3dArray
static void listToPoint3dArray(pybind11::list lst, AcGePoint3dArray& out) {
    out.setLogicalLength(0);
    for (auto item : lst) {
        out.append(item.cast<AcGePoint3d>());
    }
}

// 辅助：AcDbIntArray → pybind11::list
static pybind11::list intArrayToList(const AcDbIntArray& arr) {
    pybind11::list lst;
    for (int i = 0; i < arr.length(); i++) {
        lst.append(arr[i]);
    }
    return lst;
}

// 辅助：pybind11::list → AcDbIntArray
static void listToIntArray(pybind11::list lst, AcDbIntArray& out) {
    out.setLogicalLength(0);
    for (auto item : lst) {
        out.append(item.cast<int>());
    }
}

bool PyDbGripOverrule::isApplicable(const AcRxObject* pOverruledSubject) const {
    if (!reg_isApplicable) return false;
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
        catch (...) { reg_isApplicable = false; }
    }
    return true;
}

Acad::ErrorStatus PyDbGripOverrule::getGripPoints(const AcDbEntity* pSubject, AcGePoint3dArray& gripPoints, AcDbIntArray& osnapModes, AcDbIntArray& geomIds) {
    if (!reg_getGripPoints) return eNotImplementedYet;
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "getGripPoints")) {
        try {
            pybind11::list pyGrip = point3dArrayToList(gripPoints);
            pybind11::list pyOsnap = intArrayToList(osnapModes);
            pybind11::list pyGeom = intArrayToList(geomIds);
            auto es = py_self.attr("getGripPoints")(pybind11::cast(pSubject), pyGrip, pyOsnap, pyGeom).cast<Acad::ErrorStatus>();
            if (es == eOk) {
                listToPoint3dArray(pyGrip, gripPoints);
                listToIntArray(pyOsnap, osnapModes);
                listToIntArray(pyGeom, geomIds);
            }
            return es;
        }
        catch (const pybind11::error_already_set&) { PyErr_Clear(); }
        catch (...) { reg_getGripPoints = false; }
    }
    return AcDbGripOverrule::getGripPoints(pSubject, gripPoints, osnapModes, geomIds);
}

Acad::ErrorStatus PyDbGripOverrule::moveGripPointsAt(AcDbEntity* pSubject, const AcDbIntArray& indices, const AcGeVector3d& offset) {
    if (!reg_moveGripPointsAt) return eNotImplementedYet;
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "moveGripPointsAt")) {
        try {
            pybind11::list pyIndices = intArrayToList(indices);
            return py_self.attr("moveGripPointsAt")(pybind11::cast(pSubject), pyIndices, offset).cast<Acad::ErrorStatus>();
        }
        catch (const pybind11::error_already_set&) { PyErr_Clear(); }
        catch (...) { reg_moveGripPointsAt = false; }
    }
    return AcDbGripOverrule::moveGripPointsAt(pSubject, indices, offset);
}

Acad::ErrorStatus PyDbGripOverrule::getStretchPoints(const AcDbEntity* pSubject, AcGePoint3dArray& stretchPoints) {
    if (!reg_getStretchPoints) return eNotImplementedYet;
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "getStretchPoints")) {
        try {
            pybind11::list pyStretch = point3dArrayToList(stretchPoints);
            auto es = py_self.attr("getStretchPoints")(pybind11::cast(pSubject), pyStretch).cast<Acad::ErrorStatus>();
            if (es == eOk) {
                listToPoint3dArray(pyStretch, stretchPoints);
            }
            return es;
        }
        catch (const pybind11::error_already_set&) { PyErr_Clear(); }
        catch (...) { reg_getStretchPoints = false; }
    }
    return AcDbGripOverrule::getStretchPoints(pSubject, stretchPoints);
}

Acad::ErrorStatus PyDbGripOverrule::moveStretchPointsAt(AcDbEntity* pSubject, const AcDbIntArray& indices, const AcGeVector3d& offset) {
    if (!reg_moveStretchPointsAt) return eNotImplementedYet;
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "moveStretchPointsAt")) {
        try {
            pybind11::list pyIndices = intArrayToList(indices);
            return py_self.attr("moveStretchPointsAt")(pybind11::cast(pSubject), pyIndices, offset).cast<Acad::ErrorStatus>();
        }
        catch (const pybind11::error_already_set&) { PyErr_Clear(); }
        catch (...) { reg_moveStretchPointsAt = false; }
    }
    return AcDbGripOverrule::moveStretchPointsAt(pSubject, indices, offset);
}

void PyDbGripOverrule::gripStatus(AcDbEntity* pSubject, const AcDb::GripStat status) {
    if (!reg_gripStatus) return;
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "gripStatus")) {
        try {
            py_self.attr("gripStatus")(pybind11::cast(pSubject), (int)status);
            return;
        }
        catch (const pybind11::error_already_set&) { PyErr_Clear(); }
        catch (...) { reg_gripStatus = false; }
    }
    AcDbGripOverrule::gripStatus(pSubject, status);
}

void bind_PyDbGripOverrule(pym& m) {
    pybind11::class_<PyDbGripOverrule>(m, "GripOverrule",
        "夹点 Overrule。Python 子类继承并重写方法以覆盖实体夹点行为。\n"
        "重写示例：\n"
        "class MyGripOverrule(GripOverrule):\n"
        "    def isApplicable(self, obj): return True\n"
        "    def getGripPoints(self, entity, gripPoints, osnapModes, geomIds):\n"
        "        gripPoints.append(Point3d(0,0,0))  # 添加自定义夹点\n"
        "        return eOk")
        .def(pybind11::init<>())
        .def_readwrite("regIsApplicable", &PyDbGripOverrule::reg_isApplicable, "是否注册 isApplicable 回调")
        .def_readwrite("regGetGripPoints", &PyDbGripOverrule::reg_getGripPoints, "是否注册 getGripPoints 回调")
        .def_readwrite("regMoveGripPointsAt", &PyDbGripOverrule::reg_moveGripPointsAt, "是否注册 moveGripPointsAt 回调")
        .def_readwrite("regGetStretchPoints", &PyDbGripOverrule::reg_getStretchPoints, "是否注册 getStretchPoints 回调")
        .def_readwrite("regMoveStretchPointsAt", &PyDbGripOverrule::reg_moveStretchPointsAt, "是否注册 moveStretchPointsAt 回调")
        .def_readwrite("regGripStatus", &PyDbGripOverrule::reg_gripStatus, "是否注册 gripStatus 回调");
}
