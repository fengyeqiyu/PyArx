#pragma once
#include "stdafx.h"

// ===========================================================================
// PyDispatchBinder —— 把 PyAppDispatcher（上下文投递）绑定到 pybind 的 Dispatch 子模块
//
// 上下文投递归属 PyArxEngine（python 环境 + 重定向 + 命令管理 + 上下文调度）。
// PyAppDispatcher 与本绑定实现（PyDispatchBinder）编译进 PyArxEngine，由 PyEngine
// 在 import PyArxCore 后调用 bind_PyDispatch 挂载 PyArxEngine.Dispatch 子模块
// （与 Redir / Cmd 同模式）。DispatchContext 枚举来自 common/PyDispatchFlags.h。
//
// 供 Python 脚本从 modeless 面板/窗体按钮回调把任意 callable 投递到 AutoCAD 的
// application / command context 执行（按钮回调本身只投递、不碰数据库，从而规避
// idle 非命令上下文 lockDocument 死锁）：
//   import PyArxEngine.Dispatch as Dispatch
//   Dispatch.RunInCommandContext(func, *args)          # 异步，命令上下文（推荐）
//   Dispatch.BeginRunInApplicationContext(func, *args) # 异步，应用上下文
//   Dispatch.RunInApplicationContext(func, *args)      # 同步，应用上下文
//
// 遵循项目规范：
//   - 绑定参数用真实 C++ 类型 + 原生 py::function/py::args，不做多余 py::object；
//   - SDK / Python 调用失败不抛异常、不崩溃（RunOneTrampoline 内已兜底）；
//   - DispatchContext 枚举以 Context 命名暴露，Python 侧可用作精确投递控制。
// ===========================================================================
void bind_PyDispatch(pym& m);
