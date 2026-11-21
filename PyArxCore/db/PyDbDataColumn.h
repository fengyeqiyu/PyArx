#pragma once
#include "stdafx.h"

// AcDbDataColumn 的 Python 封装
class PyDbDataColumn {
public:
    PyDbDataColumn() = default;
};

void bind_PyDbDataColumn(pym& m);