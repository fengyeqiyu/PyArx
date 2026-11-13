// PyMfcCore 的 DLL 入口：标准 MFC 扩展模块初始化（纯 GUI 层，不引 ObjectArx 头）。
// 必须初始化扩展模块，否则 new CDynLinkLibrary 缺失会让 CWinApp/CWnd 的
// 模块态（AfxGetAppModuleState）为空，控件创建时抛 AfxThrowMemoryException。
// 编码：UTF-8 带 BOM。
#ifndef _AFXEXT
#define _AFXEXT
#endif

#include "stdafx.h"
#include <afxdllx.h>

static AFX_EXTENSION_MODULE PyMfcCoreDLL = { FALSE, nullptr, nullptr, nullptr, nullptr };

#pragma comment(linker, "/FORCE:MULTIPLE")

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        if (!AfxInitExtensionModule(PyMfcCoreDLL, hInstance))
            return 0;
        // 把自己挂进 MFC 的 CDynLinkLibrary 链，导出本模块的共享类/资源
        new CDynLinkLibrary(PyMfcCoreDLL);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        AfxTermExtensionModule(PyMfcCoreDLL);
    }
    return 1;
}
