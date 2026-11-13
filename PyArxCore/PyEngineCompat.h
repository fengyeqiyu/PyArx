#pragma once
// PyEngineCompat.h — PyArxCore.pyd 不依赖 PyArxEngine.arx，提供 PyEngine 调试日志桩。
// Core.pyd 是独立扩展模块，不链接 Engine；dbgLog/arxHandle 在此降级为 no-op。
// 编码：UTF-8 带 BOM。

#include <windows.h>

// PyEngine 命名空间桩：dbgLog 静默，arxHandle 返回 nullptr
namespace PyEngine {
    inline void dbgLog(const wchar_t*, ...) {}
    inline HMODULE arxHandle() { return nullptr; }
}

// C 接口桩：PyErrorStatusException.h 用 PyEngine_DbgLog
inline void PyEngine_DbgLog(const wchar_t*, ...) {}
