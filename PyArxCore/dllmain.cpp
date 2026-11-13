//-----------------------------------------------------------------------------
// dllmain.cpp — PyArxCore.pyd 的 MFC 扩展模块入口。
// PyArxCore 现已接入 MFC + AcUi（PyApp 门面 / PyPaletteSet 侧栏需要），
// 必须补 AfxWinInit + AC_IMPLEMENT_EXTENSION_MODULE，
// 否则 new CAdUiPaletteSet() 会因模块态 MFC 堆未初始化抛 AfxThrowMemoryException。
// 编码：UTF-8 带 BOM。
//-----------------------------------------------------------------------------
#ifndef _AFXEXT
#define _AFXEXT
#endif
#include <afxwin.h>
#include <afxdllx.h>
#include "AcExtensionModule.h"

// 实例化本 pyd 的扩展模块
AC_IMPLEMENT_EXTENSION_MODULE(PyArxCoreDLL)

#pragma comment(linker, "/FORCE:MULTIPLE")

//-----------------------------------------------------------------------------
// DLL 入口
//-----------------------------------------------------------------------------
extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    if (dwReason == DLL_PROCESS_ATTACH) {
        PyArxCoreDLL.AttachInstance(hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        PyArxCoreDLL.DetachInstance();
    }
    return 1;
}
