// PyMfcXml 的 DLL 入口。去掉 MFC 扩展模块初始化后，只剩进程挂接通知：
// shim 自己注册窗口类、自己走 WndProc thunk，不再需要 CDynLinkLibrary / 模块态。
#include "stdafx.h"

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);
    if (dwReason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(hInstance);
    return 1;
}
