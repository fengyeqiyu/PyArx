#pragma once
#include "stdafx.h"

// AcDbAuditInfo 的 Python 封装
class PyDbAuditInfo {
public:
    PyDbAuditInfo() = default;
};

void bind_PyDbAuditInfo(pym& m);