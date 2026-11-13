#include "stdafx.h"
#include "PyDbDataColumn.h"

void bind_PyDbDataColumn(pym& m) {
    pybind11::class_<PyDbDataColumn>(m, "DataColumn",
        "AcDbDataColumn 封装")
        .def(pybind11::init<>());
}