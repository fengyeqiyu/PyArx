#include "stdafx.h"
#include "PyDbDimensionStyleOverrule.h"

void bind_PyDbDimensionStyleOverrule(pym& m) {
    pybind11::class_<PyDbDimensionStyleOverrule>(m, "DimensionStyleOverrule",
        "AcDbDimensionStyleOverrule 封装")
        .def(pybind11::init<>());
}