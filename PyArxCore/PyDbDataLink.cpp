#include "stdafx.h"
#include "PyDbDataLink.h"

void bind_PyDbDataLink(pym& m) {
    pybind11::class_<PyDbDataLink>(m, "DataLink",
        "AcDbDataLink 封装")
        .def(pybind11::init<>());
}