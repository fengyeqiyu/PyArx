#include "stdafx.h"
#include "PyDbObjectOverrule.h"
#include "PyDbObject.h"

// 说明（原设计，本批 H3 不改变）：Python 回调抛异常时，PyErr_Clear / 吞掉异常后
// 落到下面 SDK 基类默认实现。这是历史既有行为；回调异常语义（拒绝 vs 回退默认）
// 留待后续 Overrule 策略统一时再评估，本批仅补 cast_error 守卫，不调整回退路径。

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
        catch (const pybind11::error_already_set&) { PyErr_Clear(); }
        catch (...) {}
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
        catch (const pybind11::error_already_set&) { PyErr_Clear(); }
        catch (...) {}
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
        catch (const pybind11::error_already_set&) { PyErr_Clear(); }
        catch (...) {}
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
        catch (const pybind11::error_already_set&) { PyErr_Clear(); }
        catch (...) {}
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
        catch (const pybind11::error_already_set&) { PyErr_Clear(); }
        catch (...) {}
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
