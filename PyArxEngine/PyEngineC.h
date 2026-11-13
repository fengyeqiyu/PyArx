#pragma once

// PyEngineC.h — PyArxEngine.dll 暴露给 Core.arx 的纯 C 接口。
// Core.arx 通过 PyArxEngine.lib 导入库解析这些函数，不直接链接 python315t.lib。
// 编码：UTF-8 带 BOM。

#ifdef __cplusplus
extern "C" {
#endif

// 初始化 Python 解释器（懒初始化，幂等）。返回 0=成功，非 0=失败。
extern int PyEngine_Init(void);

// 运行 Python 脚本文件。返回 0=成功。
extern int PyEngine_RunScript(const wchar_t* scriptPath);

// 执行 Python 代码字符串。返回 0=成功。
extern int PyEngine_ExecString(const wchar_t* code);

// pip 包管理。operation=install/uninstall/list/show/upgrade/download。
// packages: 包名数组（UTF-16），count: 包数量。返回 pip 退出码（0=成功，-1=失败）。
extern int PyEngine_PipRun(const wchar_t* operation, const wchar_t* const* packages, int count);

// 释放引擎（卸载时调用）。
extern void PyEngine_Release(void);

// 设置 ARX 模块句柄（kInitAppMsg 传入）。
extern void PyEngine_SetArxHandle(void* hMod);

#ifdef __cplusplus
}
#endif
