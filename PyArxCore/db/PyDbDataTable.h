#pragma once
#include "stdafx.h"

// AcDbDataTable 的 Python 封装
class PyDbDataTable {
public:
    PyDbDataTable() = default;
};

void bind_PyDbDataTable(pym& m);