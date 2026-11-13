#pragma once
// RegisterPyMfcControls —— 把全部标准 MFC 控件注册到 XmlControlRegistry。
// 供 PyMfc 模块初始化时调用一次（PYBIND11_MODULE）。
// 编码：本文件 UTF-8 with BOM。

void RegisterPyMfcControls();
