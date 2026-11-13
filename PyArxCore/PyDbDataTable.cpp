#include "stdafx.h"
#include "PyDbDataTable.h"

void bind_PyDbDataTable(pym& m) {
    pybind11::class_<PyDbDataTable>(m, "DataTable",
        "AcDbDataTable 封装")
        .def(pybind11::init<>());
}