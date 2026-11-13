#pragma once
// PyMfcCore 的 XAML 加载器 Python 绑定入口。
// 供 PyMfcCore.cpp 的 PYBIND11_MODULE 调用，注册 PyMfcCore.xaml 子模块。
// 编码：本文件 UTF-8 with BOM。

#include <pybind11/pybind11.h>

void RegisterXamlBindings(pybind11::module_& m);
