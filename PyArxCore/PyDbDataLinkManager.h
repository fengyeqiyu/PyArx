#pragma once
#include "stdafx.h"

// AcDbDataLinkManager 的 Python 封装
class PyDbDataLinkManager {
public:
    PyDbDataLinkManager() = default;
};

void bind_PyDbDataLinkManager(pym& m);