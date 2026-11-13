// PyUt.h - PyUt 模块公共声明：封装 acut* 通用工具函数（命令行输出、通配匹配、
// 字符串处理、ads 几何、单位换算、字符判定）。纯函数，无状态。
#pragma once

#include "stdafx.h"
#include <pybind11/pybind11.h>

// PyArx.Core 子模块入口：通用工具函数（命令行输出、通配匹配、字符串、ads 几何、单位换算、字符判定、gen_stub），由 PyArxBinder.cpp 调用。
void bind_PyCore(pym& m);
// PyArx.Utility 子模块入口：模块管理（pyreload 热重载），由 PyArxBinder.cpp 调用。
void bind_PyUtility(pym& m);
// PyArx.Debug 子模块入口：脚本断点调试（debugpy，支持 VSCode Attach），由 PyArxBinder.cpp 调用。
void bind_PyDebug(pym& m);
