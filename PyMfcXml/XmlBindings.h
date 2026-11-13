#pragma once
// PyMfcCore 的 XML 加载器 Python 绑定入口。
// 供 PyMfcCore.cpp 的 PYBIND11_MODULE 调用，注册 PyMfcCore.xml 子模块。
// 编码：本文件 UTF-8 with BOM。

#include <pybind11/pybind11.h>

void RegisterXmlBindings(pybind11::module_& m);
