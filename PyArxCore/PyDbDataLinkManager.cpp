#include "stdafx.h"
#include "PyDbDataLinkManager.h"

void bind_PyDbDataLinkManager(pym& m) {
    pybind11::class_<PyDbDataLinkManager>(m, "DataLinkManager",
        "AcDbDataLinkManager 封装")
        .def(pybind11::init<>());
}