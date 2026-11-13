#pragma once
// MFCWebViewApi.h —— MFCWebView.dll 的 C ABI 门面。
//
// 为什么走 C 而不是直接导出 C++ 类：DLL 边界上不能传 STL / MFC 对象
// （跨模块 CRT、异常类型、对象布局都会打架）。这里只传不透明句柄和
// 普通 C 字符串，回调也只给函数指针 + void*，宿主里想要 std::function
// 就在自己那侧包一层（WebShell.h 已经包好了）。
//
// 调用方只需要 windows.h + 本头文件；WebView2 / MFC 的实现细节全在 DLL 内部。
#include <windows.h>

#ifdef MFCWEB_BUILD_DLL
#define MFCWEB_API __declspec(dllexport)
#else
#define MFCWEB_API __declspec(dllimport)
#endif

extern "C"
{
    // 不透明句柄：一个 WebShell 实例
    typedef void* mfcweb_shell;

    // 页面就绪回调（WebView2 控制器建好、页面可交互）
    typedef void (*mfcweb_ready_fn)(void* user);

    // 页面消息回调：返回 1 表示该消息已被处理、不再走 DLL 内置路由
    typedef int (*mfcweb_command_fn)(void* user, const wchar_t* json);

    MFCWEB_API mfcweb_shell mfcweb_create(void);
    MFCWEB_API void        mfcweb_destroy(mfcweb_shell s);

    // host：承载窗口（客户区整块交给 WebView2）。失败时 err 里带原因
    MFCWEB_API int  mfcweb_attach(mfcweb_shell s, HWND host, const wchar_t* userDataDir,
                                  wchar_t* err, int errCch);
    MFCWEB_API void mfcweb_detach(mfcweb_shell s);

    MFCWEB_API int mfcweb_load_page(mfcweb_shell s, const wchar_t* htmlPath);
    MFCWEB_API int mfcweb_load_url(mfcweb_shell s, const wchar_t* url);
    MFCWEB_API int mfcweb_emit(mfcweb_shell s, const wchar_t* json);
    MFCWEB_API int mfcweb_eval(mfcweb_shell s, const wchar_t* js);

    // 无边框窗口四边拖拽改尺寸的命中宽度（像素），0 = 关闭
    MFCWEB_API void mfcweb_set_resize_border(mfcweb_shell s, int px);

    MFCWEB_API void mfcweb_set_ready_cb(mfcweb_shell s, mfcweb_ready_fn fn, void* user);
    MFCWEB_API void mfcweb_set_command_cb(mfcweb_shell s, mfcweb_command_fn fn, void* user);

    MFCWEB_API HWND mfcweb_host(mfcweb_shell s);
    MFCWEB_API int  mfcweb_is_ready(mfcweb_shell s);
    MFCWEB_API double mfcweb_scale(mfcweb_shell s);

    // 版本串（形如 "MFCWebView 1.0 / WebView2"），便于宿主排查装的是哪份 DLL
    MFCWEB_API const wchar_t* mfcweb_version(void);
}
