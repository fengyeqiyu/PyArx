#include "stdafx.h"
#include "PyDbDictionaryIterator.h"

void bind_PyDbDictionaryIterator(pym& m) {
    pybind11::class_<PyDbDictionaryIterator>(m, "DictionaryIterator",
        "AcDbDictionaryIterator 封装")
        .def(pybind11::init<>());
}