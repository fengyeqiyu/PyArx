#pragma once
#include "stdafx.h"

// ===========================================================================
// PyThreadScope —— 线程状态守卫（free-threaded 运行时，不使用 GIL）
//
// 本工程固定链接 python315t.dll（Py_GIL_DISABLED）：解释器里根本没有 GIL，
// 也就不存在需要 acquire/release 的全局锁。CAD 主线程在 Py_InitializeFromConfig
// 时就挂上了线程状态，后续命令、回调、idle 泵都跑在同一线程上，直接调用
// Python C API 即可。旧的 py::gil_scoped_acquire / PyGILState_Ensure 在此
// 构建下只会产生无意义的开销，统一收敛成本类的空守卫。
//
//   - Ready()：只校验当前线程是否已有线程状态（PyThreadState_GetUnchecked），
//     缺失时由调用方报错，绝不静默进 Python；
//   - 仅当显式编译非 free-threaded（未定义 Py_GIL_DISABLED）的老构建时，
//     才退回 pybind11 的 GIL 守卫，保证这份代码仍能编 GIL 版。
// ===========================================================================
#ifdef Py_GIL_DISABLED

class PyThreadScope {
public:
    PyThreadScope() = default;
    PyThreadScope(const PyThreadScope&) = delete;
    PyThreadScope& operator=(const PyThreadScope&) = delete;
    void disarm() {}
    // free-threaded：无锁可取，只回答"当前线程是否已挂接线程状态"
    static bool Ready() { return PyThreadState_GetUnchecked() != nullptr; }
};

#else

class PyThreadScope {
    py::gil_scoped_acquire m_gil;
public:
    PyThreadScope() = default;
    PyThreadScope(const PyThreadScope&) = delete;
    PyThreadScope& operator=(const PyThreadScope&) = delete;
    void disarm() { m_gil.disarm(); }
    static bool Ready() { return PyGILState_Check() != 0; }
};

#endif
