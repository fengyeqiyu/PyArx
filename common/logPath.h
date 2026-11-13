// 日志目录解析：不再把 G:\PyArx\log 写死在源码里。
// 优先级：环境变量 PYARX_LOG_DIR > 模块自身所在目录下的 log 子目录。
// 注意：ARX / pyd 都由宿主进程（acad.exe / python.exe）加载，不能用主模块路径
//       （那会落到 Program Files，通常不可写），必须按模块名取自身目录。
#pragma once

#include <windows.h>
#include <string>

inline std::wstring pyarxModuleDir(const wchar_t* moduleName)
{
    HMODULE mod = moduleName != nullptr ? ::GetModuleHandleW(moduleName) : nullptr;
    wchar_t path[MAX_PATH] = {};
    if (::GetModuleFileNameW(mod, path, MAX_PATH) == 0)
        return std::wstring(L".");

    std::wstring dir(path);
    const size_t cut = dir.find_last_of(L"\\/");
    if (cut != std::wstring::npos)
        dir.resize(cut);
    return dir;
}

inline std::wstring pyarxLogDir(const wchar_t* moduleName = L"PyArxEngine.arx")
{
    wchar_t env[1024] = {};
    const DWORD n = ::GetEnvironmentVariableW(L"PYARX_LOG_DIR", env, 1024);
    if (n >= 2)
        return std::wstring(env, n);

    std::wstring dir = pyarxModuleDir(moduleName) + L"\\log";
    ::CreateDirectoryW(dir.c_str(), nullptr);   // 已存在时失败，忽略
    return dir;
}

inline std::wstring pyarxLogFile(const wchar_t* name, const wchar_t* moduleName = L"PyArxEngine.arx")
{
    return pyarxLogDir(moduleName) + L"\\" + name;
}
