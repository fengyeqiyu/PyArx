#pragma once
#include "stdafx.h"

// AcDbBackground 的 Python 封装
class PyDbBackground {
public:
    PyDbBackground() = default;
};

void bind_PyDbBackground(pym& m);