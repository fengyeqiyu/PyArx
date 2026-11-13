#pragma once
#include "stdafx.h"

// ===========================================================================
// PyCmdBinder —— 将 PyCmd（纯静态 CAD 命令管理类）绑定到 pybind 的 Cmd 子模块
//
// 命令管理归属 PyArxEngine（python 环境 + 重定向 + 命令管理）。PyCmd 类
// （PyRxCmd）与本绑定实现（PyCmdBinder）编译进 PyArxEngine，由 PyEngine 在
// import PyArxCore 后调用 bind_PyCmd 挂载 PyArxCore.Cmd 子模块（与 Redir 同模式）。
// PyCommandFlags（CommandFlags 枚举）来自 PyArxCore 公共头，Enum 绑定仍在核心。
//
// 供 Python 脚本用装饰器注册 / 移除 / 清空 AutoCAD 命令（命令执行 = 运行被装饰函数）：
//   from PyArxEngine.Cmd import PyCmd, CommandFlags
//   @PyCmd.command                              # 直接用函数名注册
//   def MYFUNC(): ...
//   @PyCmd.command("MYCMD", flags=CommandFlags.UsePickSet)
//   def myfunc(): ...
//   PyCmd.removeCmd("MYCMD")   /   PyCmd.clear()
//
// 遵循项目规范：
//   - addCmd 成功返回 Acad::ErrorStatus（已绑定为 Enum.ErrorStatus）；
//   - Python callable → std::function<void()> 由 pybind11 functional.h 自动转换；
//   - 绑定参数用真实 C++ 类型（AcString / int），避免 py::object。
// ===========================================================================
void bind_PyCmd(pym& m);
