#pragma once
#include "stdafx.h"

// AcDbDictionaryIterator 的 Python 封装
class PyDbDictionaryIterator {
public:
    PyDbDictionaryIterator() = default;
};

void bind_PyDbDictionaryIterator(pym& m);