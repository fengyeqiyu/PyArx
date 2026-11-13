//-----------------------------------------------------------------------------
// dllmain.cpp — AutoCAD ARX MFC 扩展模块入口（参照 PyRx dllmain.cpp / AcExtensionModule.h）。
// 工程全局定义 _AFXEXT（MFC 扩展 DLL 模式），MFC 不注入 dllmodul.obj 的 per-DLL DllMain 代理，
// 由本文件提供 DllMain。AC_IMPLEMENT_EXTENSION_MODULE 同时定义
// CAcModuleResourceOverride::m_extensionModule 静态（否则该符号未解析）。
// 不使用 stdafx PCH。UTF-8 带 BOM。
//-----------------------------------------------------------------------------
// _AFXEXT：MFC 扩展 DLL 模式（仅本 TU 内定义，避免污染 stdafx PCH）。
// 使 afx.h 走扩展 DLL 的 lib 链接策略，不注入 dllmodul.obj 的 per-DLL DllMain 代理。
#ifndef _AFXEXT
#define _AFXEXT
#endif
#include <afxwin.h>
#include <afxdllx.h>
#include "AcExtensionModule.h"

// 实例化本 ARX 的扩展模块（全工程唯一一处）
AC_IMPLEMENT_EXTENSION_MODULE(PyArxDLL)

// 兼容处理：UseOfMfc=Dynamic 自动拉入 mfcs140u.lib(dllmodul.obj) 的 DllMain，
// 与本文件 DllMain 在链接期 LNK2005。两者均为 DLL_PROCESS_ATTACH/DETACH 初始化桩
// （ARX 寄生 acad 进程，消息循环由 acad 驱动），任取其一即可，故允许多重定义择一。
#pragma comment(linker, "/FORCE:MULTIPLE")

//-----------------------------------------------------------------------------
// DLL 入口
//-----------------------------------------------------------------------------
extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    if (dwReason == DLL_PROCESS_ATTACH) {
        PyArxDLL.AttachInstance(hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        PyArxDLL.DetachInstance();
    }
    return 1;   // ok
}
