#pragma once
// WebShellCore.h —— DLL 内部的真实实现类（不对外暴露）。
//
// 历史：这个类原来叫 webmfc::WebShell，直接以静态库形式进两个 pyd。
// 改成 DLL 之后，它对外的入口换成了 MFCWebViewApi.h 里的 C 函数，
// 类本身留在 DLL 内部，宿主拿不到也不需要它的布局。
#include <windows.h>
#include <functional>
#include <string>
#include "WebViewHost.h"

namespace webmfc
{
    class Shell
    {
    public:
        Shell();
        ~Shell();

        bool attach(HWND host, const std::wstring& userDataDir, std::wstring& err);
        void detach();

        bool loadPage(const std::wstring& htmlPath);
        bool loadUrl(const std::wstring& url);

        bool emit(const std::wstring& json);
        bool eval(const std::wstring& js);

        void setResizeBorder(int px);

        void setReadyHandler(std::function<void()> handler);
        void setCommandHandler(std::function<bool(const std::wstring& json)> handler);

        HWND host() const;

        WebViewHost& view();

    private:
        struct Impl;
        Impl* m_impl = nullptr;

        Shell(const Shell&) = delete;
        Shell& operator=(const Shell&) = delete;
    };
}
