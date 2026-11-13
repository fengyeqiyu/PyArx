#pragma once
#include "stdafx.h"

// AcDbDataLink 的 Python 封装
class PyDbDataLink {
public:
    PyDbDataLink() = default;
};

void bind_PyDbDataLink(pym& m);