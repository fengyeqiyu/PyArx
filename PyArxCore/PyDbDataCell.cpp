#include "stdafx.h"
#include "PyDbDataCell.h"

void bind_PyDbDataCell(pym& m) {
    pybind11::class_<PyDbDataCell>(m, "DataCell",
        "AcDbDataCell 封装")
        .def(pybind11::init<>());
}