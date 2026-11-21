#pragma once
#include "stdafx.h"

// AcDbDataCell 的 Python 封装
class PyDbDataCell {
public:
    PyDbDataCell() = default;
};

void bind_PyDbDataCell(pym& m);