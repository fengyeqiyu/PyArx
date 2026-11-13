#include "stdafx.h"
#include "PyDispatchBinder.h"
#include "PyAppDispatcher.h"

// ---------------------------------------------------------------------------
// 将 PyAppDispatcher 绑定到 pybind 的 Dispatch 子模块。
// 上下文投递归属 PyArxEngine：本文件编译进 PyArxEngine，bind_PyDispatch 由 PyEngine
// 在 import PyArxCore 后调用，挂载 PyArxEngine.Dispatch 子模块（与 Redir/Cmd 同模式）。
// DispatchContext 枚举定义于 common/PyDispatchFlags.h，绑定在此（归属引擎）。
// 编码：UTF-8 带 BOM。
// ---------------------------------------------------------------------------
void bind_PyDispatch(pym& m) {
    // DispatchContext 枚举：三种投递语义（Application 同步 / BeginApplication 异步 /
    // BeginCommand 异步-命令上下文）。export_values 让 Python 侧可直接用
    // Dispatch.Application 等裸名。
    py::enum_<DispatchContext>(m, "Context",
        "上下文投递语义（决定 Python callable 被 AcApDocManager 调度到哪种上下文执行）")
        .value("Application",      DispatchContext::Application,
            "同步：executeInApplicationContext，阻塞到完成")
        .value("BeginApplication", DispatchContext::BeginApplication,
            "异步：beginExecuteInApplicationContext，立即返回")
        .value("BeginCommand",     DispatchContext::BeginCommand,
            "异步：beginExecuteInCommandContext，当作一条新命令排队（推荐，文档自动拿命令锁）")
        .export_values();

    // 同步：application context 执行，阻塞到 func(*args) 返回。
    m.def("RunInApplicationContext",
        [](py::function func, py::args args) {
            PyAppDispatcher::Instance().Dispatch(func, args, DispatchContext::Application);
        },
        py::arg("func"),
        "同步：在 application context 执行 func(*args)，阻塞到完成。");

    // 异步：application context，排入队列立即返回。
    m.def("BeginRunInApplicationContext",
        [](py::function func, py::args args) {
            PyAppDispatcher::Instance().Dispatch(func, args, DispatchContext::BeginApplication);
        },
        py::arg("func"),
        "异步：把 func(*args) 排到 application context 队列，立即返回。");

    // 异步：命令上下文（推荐）。把 func(*args) 当成一条新命令排队，文档自动拿命令锁。
    m.def("RunInCommandContext",
        [](py::function func, py::args args) {
            PyAppDispatcher::Instance().Dispatch(func, args, DispatchContext::BeginCommand);
        },
        py::arg("func"),
        "异步：把 func(*args) 当成一条新命令排队执行（推荐，文档自动拿命令锁，PyDbTrans() 安全）。");
}
