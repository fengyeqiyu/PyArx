#include "stdafx.h"
#include "PyDbBackground.h"

void bind_PyDbBackground(pym& m) {
    pybind11::class_<PyDbBackground>(m, "Background",
        "AcDbBackground 封装")
        .def(pybind11::init<>());
}