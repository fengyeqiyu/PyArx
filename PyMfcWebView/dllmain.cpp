// PyMfcWebView 的 DLL 入口：标准 MFC 扩展模块初始化。
// 不初始化扩展模块的话，CWinApp/CWnd 的模块态（AfxGetAppModuleState）为空，
// 在裸 python.exe 里建窗会直接抛 AfxThrowMemoryException 或访问空指针。
#ifndef _AFXEXT
#define _AFXEXT
#endif

#include "stdafx.h"
#include <afxdllx.h>

static AFX_EXTENSION_MODULE PyMfcWebViewDLL = { FALSE, nullptr, nullptr, nullptr, nullptr };

#pragma comment(linker, "/FORCE:MULTIPLE")

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        if (!AfxInitExtensionModule(PyMfcWebViewDLL, hInstance))
            return 0;
        new CDynLinkLibrary(PyMfcWebViewDLL);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        // 不在 DETACH 里调 AfxTermExtensionModule：pyd 被 python 在解释器收尾时卸载，
        // 那时 MFC 运行库已经开始拆状态，再去动扩展模块链表会一起把进程打死（0xC0000409）。
        // 进程本来就是退出中，扩展模块的内存不需要"优雅回收"。
    }
    return 1;
}
