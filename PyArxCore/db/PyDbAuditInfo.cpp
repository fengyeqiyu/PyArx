#include "stdafx.h"
#include "PyDbAuditInfo.h"

void bind_PyDbAuditInfo(pym& m) {
    pybind11::class_<PyDbAuditInfo>(m, "AuditInfo",
        "AcDbAuditInfo 封装")
        .def(pybind11::init<>());
}