// PyEngineC.cpp — PyArxEngine.dll 纯 C 接口实现，包装 PyEngine C++ 单例。
// Core.arx 通过 PyArxEngine.lib 导入库调用本文件导出的 C 函数。
// 编码：UTF-8 带 BOM。

#include "stdafx.h"
#include "PyEngine.h"
#include "PyEngineC.h"
#include <acedads.h>
#include <acutads.h>

extern "C" {

int PyEngine_Init(void) {
    try {
        PyEngine::instance();
        return 0;
    }
    catch (const std::exception& e) {
        acutPrintf(L"\n[PyArx] PyEngine_Init C++ 异常(std): %s\n", AcString(e.what(), AcString::Utf8));
        return 1;
    }
    catch (const std::string& s) {
        acutPrintf(L"\n[PyArx] PyEngine_Init C++ 异常(string): %hs\n", s.c_str());
        return 1;
    }
    catch (const wchar_t* w) {
        acutPrintf(L"\n[PyArx] PyEngine_Init C++ 异常(wchar): %ls\n", w ? w : L"(null)");
        return 1;
    }
    catch (...) {
        acutPrintf(L"\n[PyArx] PyEngine_Init C++ 未知异常(...)\n");
        return 1;
    }
}

int PyEngine_RunScript(const wchar_t* scriptPath) {
    try {
        auto* e = PyEngine::instance();
        if (!e) return 1;
        bool ok = e->RunScript(std::filesystem::path(scriptPath));
        return ok ? 0 : 1;
    }
    catch (...) {
        return 1;
    }
}

int PyEngine_ExecString(const wchar_t* code) {
    try {
        auto* e = PyEngine::instance();
        if (!e) return 1;
        bool ok = e->ExecString(AcString(code));
        return ok ? 0 : 1;
    }
    catch (...) {
        return 1;
    }
}

int PyEngine_PipRun(const wchar_t* operation, const wchar_t* const* packages, int count) {
    try {
        auto* e = PyEngine::instance();
        if (!e) return -1;
        std::vector<AcString> pkgs;
        for (int i = 0; i < count; ++i) {
            pkgs.push_back(AcString(packages[i]));
        }
        return e->pipRun(AcString(operation), pkgs);
    }
    catch (...) {
        return -1;
    }
}

void PyEngine_Release(void) {
    try {
        PyEngine::release();
    }
    catch (...) {
    }
}

void PyEngine_SetArxHandle(void* hMod) {
    PyEngine::setArxHandle(reinterpret_cast<HMODULE>(hMod));
}

} // extern "C"
