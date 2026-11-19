#include "stdafx.h"
#include "PyDbObjectReactor.h"
#include "PyDbObject.h"

// 所有权说明（待 H1/H6 统一处理，本批 H3 不改动）：回调内临时包装 PyDbObject(pObj)
// 当前按驻留对象处理；pObj 生命周期由 AutoCAD 回调方负责，本包装不应 close/delete。
// 统一所有权模型落地前，此处保持现状，不引入新的过早 close。

void PyDbObjectReactor::cancelled(const AcDbObject* pObj) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "cancelled")) {
        try { py_self.attr("cancelled")(PyDbObject(const_cast<AcDbObject*>(pObj))); }
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    } else {
        AcDbObjectReactor::cancelled(pObj);
    }
}

void PyDbObjectReactor::copied(const AcDbObject* pOld, const AcDbObject* pNew) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "copied")) {
        try { py_self.attr("copied")(PyDbObject(const_cast<AcDbObject*>(pOld)), PyDbObject(const_cast<AcDbObject*>(pNew))); }
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    } else {
        AcDbObjectReactor::copied(pOld, pNew);
    }
}

void PyDbObjectReactor::erased(const AcDbObject* pObj, bool bErasing) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "erased")) {
        try { py_self.attr("erased")(PyDbObject(const_cast<AcDbObject*>(pObj)), bErasing); }
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    } else {
        AcDbObjectReactor::erased(pObj, bErasing);
    }
}

void PyDbObjectReactor::goodbye(const AcDbObject* pObj) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "goodbye")) {
        try { py_self.attr("goodbye")(PyDbObject(const_cast<AcDbObject*>(pObj))); }
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    } else {
        AcDbObjectReactor::goodbye(pObj);
    }
}

void PyDbObjectReactor::openedForModify(const AcDbObject* pObj) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "openedForModify")) {
        try { py_self.attr("openedForModify")(PyDbObject(const_cast<AcDbObject*>(pObj))); }
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    } else {
        AcDbObjectReactor::openedForModify(pObj);
    }
}

void PyDbObjectReactor::modified(const AcDbObject* pObj) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "modified")) {
        try { py_self.attr("modified")(PyDbObject(const_cast<AcDbObject*>(pObj))); }
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    } else {
        AcDbObjectReactor::modified(pObj);
    }
}

void PyDbObjectReactor::subObjModified(const AcDbObject* pObj, const AcDbObject* pSub) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "subObjModified")) {
        try { py_self.attr("subObjModified")(PyDbObject(const_cast<AcDbObject*>(pObj)), PyDbObject(const_cast<AcDbObject*>(pSub))); }
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    } else {
        AcDbObjectReactor::subObjModified(pObj, pSub);
    }
}

void PyDbObjectReactor::modifyUndone(const AcDbObject* pObj) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "modifyUndone")) {
        try { py_self.attr("modifyUndone")(PyDbObject(const_cast<AcDbObject*>(pObj))); }
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    } else {
        AcDbObjectReactor::modifyUndone(pObj);
    }
}

void PyDbObjectReactor::modifiedXData(const AcDbObject* pObj) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "modifiedXData")) {
        try { py_self.attr("modifiedXData")(PyDbObject(const_cast<AcDbObject*>(pObj))); }
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    } else {
        AcDbObjectReactor::modifiedXData(pObj);
    }
}

void PyDbObjectReactor::unappended(const AcDbObject* pObj) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "unappended")) {
        try { py_self.attr("unappended")(PyDbObject(const_cast<AcDbObject*>(pObj))); }
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    } else {
        AcDbObjectReactor::unappended(pObj);
    }
}

void PyDbObjectReactor::reappended(const AcDbObject* pObj) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "reappended")) {
        try { py_self.attr("reappended")(PyDbObject(const_cast<AcDbObject*>(pObj))); }
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    } else {
        AcDbObjectReactor::reappended(pObj);
    }
}

void PyDbObjectReactor::objectClosed(const AcDbObjectId objId) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "objectClosed")) {
        try { py_self.attr("objectClosed")(static_cast<uint64_t>(objId.asOldId())); }
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    } else {
        AcDbObjectReactor::objectClosed(objId);
    }
}

void bind_PyDbObjectReactor(pym& m) {
    pybind11::class_<PyDbObjectReactor>(m, "ObjectReactor",
        "对象反应器基类。Python 子类继承并重写方法以接收事件回调。")
        .def(pybind11::init<>())
        .def("cancelled", &PyDbObjectReactor::cancelled, "对象被取消打开时回调")
        .def("copied", &PyDbObjectReactor::copied, "对象被复制时回调")
        .def("erased", &PyDbObjectReactor::erased, "对象被删除/恢复时回调")
        .def("goodbye", &PyDbObjectReactor::goodbye, "对象即将销毁时回调")
        .def("openedForModify", &PyDbObjectReactor::openedForModify, "对象打开用于修改前回调")
        .def("modified", &PyDbObjectReactor::modified, "对象被修改后回调")
        .def("subObjModified", &PyDbObjectReactor::subObjModified, "子对象被修改后回调")
        .def("modifyUndone", &PyDbObjectReactor::modifyUndone, "修改被撤销时回调")
        .def("modifiedXData", &PyDbObjectReactor::modifiedXData, "XData 被修改后回调")
        .def("unappended", &PyDbObjectReactor::unappended, "对象从数据库移除时回调")
        .def("reappended", &PyDbObjectReactor::reappended, "对象重新追加到数据库时回调")
        .def("objectClosed", &PyDbObjectReactor::objectClosed, "对象关闭时回调");
}
