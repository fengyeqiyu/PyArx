#include "stdafx.h"
#include "PyDbDwgFiler.h"

void bind_PyDbDwgFiler(pym& m) {
    pybind11::class_<PyDbDwgFiler>(m, "DwgFiler",
        "AcDbDwgFiler 封装")
        .def(pybind11::init<>());
}