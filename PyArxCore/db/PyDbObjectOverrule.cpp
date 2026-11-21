#include "stdafx.h"
#include "PyDbObjectOverrule.h"
#include "PyDbObject.h"

// 说明：Python 回调抛异常时打印并落到下面 SDK 基类默认实现（回退语义不变，
// 仅把静默吞异常改为可见报错）。

Acad::ErrorStatus PyDbObjectOverrule::open(AcDbObject* pSubject, AcDb::OpenMode mode) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "open")) {
        try {
            py_self.attr("open")(PyDbObject(pSubject), static_cast<int>(mode));
            return Acad::eOk;
        }
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    }
    return AcDbObjectOverrule::open(pSubject, mode);
}

Acad::ErrorStatus PyDbObjectOverrule::close(AcDbObject* pSubject) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "close")) {
        try {
            py_self.attr("close")(PyDbObject(pSubject));
            return Acad::eOk;
        }
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    }
    return AcDbObjectOverrule::close(pSubject);
}

Acad::ErrorStatus PyDbObjectOverrule::cancel(AcDbObject* pSubject) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "cancel")) {
        try {
            py_self.attr("cancel")(PyDbObject(pSubject));
            return Acad::eOk;
        }
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    }
    return AcDbObjectOverrule::cancel(pSubject);
}

Acad::ErrorStatus PyDbObjectOverrule::erase(AcDbObject* pSubject, Adesk::Boolean erasing) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "erase")) {
        try {
            py_self.attr("erase")(PyDbObject(pSubject), erasing == Adesk::kTrue);
            return Acad::eOk;
        }
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    }
    return AcDbObjectOverrule::erase(pSubject, erasing);
}

bool PyDbObjectOverrule::isApplicable(const AcRxObject* pOverruledSubject) const {
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
        catch (pybind11::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { if (PyErr_Occurred()) PyErr_Print(); }
    }
    return true;  // 默认适用于所有对象
}

void bind_PyDbObjectOverrule(pym& m) {
    pybind11::class_<PyDbObjectOverrule>(m, "ObjectOverrule",
        "对象 Overrule 基类。Python 子类继承并重写方法以覆盖对象的默认行为。")
        .def(pybind11::init<>())
        .def("open", &PyDbObjectOverrule::open, "覆盖对象打开行为")
        .def("close", &PyDbObjectOverrule::close, "覆盖对象关闭行为")
        .def("cancel", &PyDbObjectOverrule::cancel, "覆盖对象取消行为")
        .def("erase", &PyDbObjectOverrule::erase, "覆盖对象删除行为");
}
