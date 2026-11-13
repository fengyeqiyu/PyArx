#include "stdafx.h"
#include "PyDbHighlightOverrule.h"

bool PyDbHighlightOverrule::isApplicable(const AcRxObject* pOverruledSubject) const {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "isApplicable")) {
        try { return py_self.attr("isApplicable")(pybind11::cast(pOverruledSubject)).cast<bool>(); }
        catch (const pybind11::error_already_set&) { PyErr_Clear(); }
        catch (...) {}
    }
    return true;
}

void bind_PyDbHighlightOverrule(pym& m) {
    pybind11::class_<PyDbHighlightOverrule>(m, "HighlightOverrule",
        "HighlightOverrule 基类。Python 子类继承并重写方法以覆盖默认行为。")
        .def(pybind11::init<>());
}