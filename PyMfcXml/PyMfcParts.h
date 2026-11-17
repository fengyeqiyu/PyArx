// PyMfcParts.h —— 各编译单元之间共享的内部入口（不属于 Python 绑定接口）。
// 放在这里而不是某个 .cpp 的匿名 namespace 里，才能跨 TU 使用。

#pragma once

#include "PyMfcCommon.h"
#include "PyMfcWrap.h"

// 把 Python 工厂注册进统一控件注册表（register_control）
void RegisterPythonControl(const std::string& tag, py::object factory);

// XML 文件 → 控件树（guiload）
py::object Guiload(const std::string& path, py::object target, py::object context,
                   py::object names, int firstId, py::object palette);

// XML 字符串 → 控件树（guiload_text；同一套运行时，不落盘）
py::object GuiloadText(const std::string& xmlText, py::object target, py::object context,
                       py::object names, int firstId, py::object palette);

// 只跑布局、返回各 x:Name 矩形（xmlLayout）
py::dict XmlLayoutRects(const std::string& path, int width, int height,
                         py::object overrides);
