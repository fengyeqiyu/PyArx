// PyEdJig.cpp - AcEdJig trampoline + AcEdDragStyle bindings, registered into
// the PyArx.Ed sub-module by registerPyEdJig (no standalone module entry).
#include "stdafx.h"
#include "PyEdJig.h"
#include "PyErrorStatusException.h"

namespace {

// acquire* output params are returned as (DragStatus as int, value) tuples;
// these calls block for user input and are never invoked in self-tests.
py::tuple jigAcquireString(AcEdJig* self) {
    AcString s;
    const AcEdJig::DragStatus st = self->acquireString(s);
    return py::make_tuple(static_cast<int>(st), s);
}

py::tuple jigAcquireAngle(AcEdJig* self, const std::optional<AcGePoint3d>& basePnt) {
    double ang = 0.0;
    AcEdJig::DragStatus st;
    st = basePnt ? self->acquireAngle(ang, *basePnt) : self->acquireAngle(ang);
    return py::make_tuple(static_cast<int>(st), ang);
}

py::tuple jigAcquireDist(AcEdJig* self, const std::optional<AcGePoint3d>& basePnt) {
    double dist = 0.0;
    AcEdJig::DragStatus st;
    st = basePnt ? self->acquireDist(dist, *basePnt) : self->acquireDist(dist);
    return py::make_tuple(static_cast<int>(st), dist);
}

py::tuple jigAcquirePoint(AcEdJig* self, const std::optional<AcGePoint3d>& basePnt) {
    AcGePoint3d pt;
    AcEdJig::DragStatus st;
    st = basePnt ? self->acquirePoint(pt, *basePnt) : self->acquirePoint(pt);
    return py::make_tuple(static_cast<int>(st), pt);
}

} // namespace

void registerPyEdJig(py::module& m) {
    // Ensure the centralized JIG enums (PyArx.Enum 子模块) have their type_info
    // registered before the class methods reference them.
    py::module_::import("PyArxCore.Enum");

    // ---- AcEdDragStyle: plain value class ----
    py::class_<AcEdDragStyle>(m, "AcEdDragStyle")
        .def(py::init<>())
        .def(py::init<AcEdDragStyle::StyleType, AcEdDragStyle::StyleType>(),
            py::arg("style_type_for_original"), py::arg("style_type_for_dragged"))
        .def("styleTypeForOriginal", &AcEdDragStyle::styleTypeForOriginal)
        .def("styleTypeForDragged", &AcEdDragStyle::styleTypeForDragged)
        .def("setStyleTypeForOriginal",
            [](AcEdDragStyle& self, AcEdDragStyle::StyleType t) -> bool {
                return PyWarnIfError(self.setStyleTypeForOriginal(t), L"setStyleTypeForOriginal");
            }, py::arg("style_type"))
        .def("setStyleTypeForDragged",
            [](AcEdDragStyle& self, AcEdDragStyle::StyleType t) -> bool {
                return PyWarnIfError(self.setStyleTypeForDragged(t), L"setStyleTypeForDragged");
            }, py::arg("style_type"))
        .def(py::self == py::self)
        .def(py::self != py::self);

    // ---- AcEdJig: trampoline for Python subclassing ----
    py::class_<AcEdJig, PyEdJig>(m, "AcEdJig")
        .def(py::init<>())
        // DragStatus drives the drag loop; blocks for user input.
        .def("drag", static_cast<AcEdJig::DragStatus (AcEdJig::*)()>(&AcEdJig::drag))
        .def("drag", static_cast<AcEdJig::DragStatus (AcEdJig::*)(const AcEdDragStyle&)>(&AcEdJig::drag),
            py::arg("style"))
        // Append the jigged entity to the current database; return old-style id.
        .def("append", [](AcEdJig* self) -> uint64_t {
            return self->append().asOldId();
        })
        // acquire* family: returns (DragStatus:int, value) tuple.
        .def("acquireString", &jigAcquireString)
        .def("acquireAngle", &jigAcquireAngle, py::arg("base_pnt") = py::none())
        .def("acquireDist", &jigAcquireDist, py::arg("base_pnt") = py::none())
        .def("acquirePoint", &jigAcquirePoint, py::arg("base_pnt") = py::none())
        // Keyword list / prompt text.
        .def("keywordList", [](AcEdJig* self) { return AcString(self->keywordList()); })
        .def("setKeywordList",
            [](AcEdJig* self, const AcString& kw) { self->setKeywordList(kw.kwszPtr()); },
            py::arg("keywords"))
        .def("dispPrompt", [](AcEdJig* self) { return AcString(self->dispPrompt()); })
        .def("setDispPrompt",
            [](AcEdJig* self, const AcString& msg) { self->setDispPrompt(msg.kwszPtr()); },
            py::arg("prompt"))
        // Cursor / input-control enums (registered in PyArx.Enum).
        .def("specialCursorType", &AcEdJig::specialCursorType)
        .def("setSpecialCursorType", &AcEdJig::setSpecialCursorType, py::arg("cursor"))
        .def("userInputControls", &AcEdJig::userInputControls)
        .def("setUserInputControls",
            static_cast<void (AcEdJig::*)(AcEdJig::UserInputControls)>(&AcEdJig::setUserInputControls),
            py::arg("controls"));
}
