#pragma once
// WebShell：把宿主窗口的客户区整块交给 web 渲染，并把"只有原生才做得了"的能力
// 以 JSON 命令的形式给页面调用（窗口拖拽/最小化/最大化/关闭、文件对话框、消息框、剪贴板、DPI）。
//
// 实现位置：MFCWebView.dll（C ABI 见 MFCWebViewApi.h）。本头文件只是给宿主的
// 轻量 C++ 包装：把 std::function 回调转成 C 函数指针，句柄仍然是不透明指针，
// 所以 pyd / exe 里不再链接 MFCWebView 的 C++ 实现，也能拿到同一份 DLL。
//
// 用法：new 出来的 CWnd / CFrameWnd 建好后 attach(它的 HWND)，页面里所有控件都是 web 的，
// MFC 只负责窗口外壳与这些原生能力。
#include <windows.h>
#include <functional>
#include <string>
#include "MFCWebViewApi.h"

namespace webmfc
{
    class WebShell
    {
    public:
        WebShell() : m_h(mfcweb_create()) {}
        ~WebShell()
        {
            // 先销毁 DLL 侧实例，再让 std::function 成员析构：否则回调指针悬空
            if (m_h != nullptr)
            {
                mfcweb_destroy(m_h);
                m_h = nullptr;
            }
        }

        // host：承载窗口。整块客户区会被 WebView 铺满；侧栏场景就传侧栏那个子窗的 HWND
        bool attach(HWND host, const std::wstring& userDataDir, std::wstring& err)
        {
            err.clear();
            if (m_h == nullptr)
            {
                err = L"MFCWebView 实例创建失败（DLL 未加载？）";
                return false;
            }
            wchar_t buf[512] = { 0 };
            const int ok = mfcweb_attach(m_h, host, userDataDir.empty() ? nullptr : userDataDir.c_str(),
                                         buf, 512);
            if (ok == 0 && buf[0] != L'\0')
                err = buf;
            return ok != 0;
        }

        void detach()
        {
            if (m_h != nullptr) mfcweb_detach(m_h);
        }

        bool loadPage(const std::wstring& htmlPath)
        {
            return m_h != nullptr && mfcweb_load_page(m_h, htmlPath.c_str()) != 0;
        }

        bool loadUrl(const std::wstring& url)
        {
            return m_h != nullptr && mfcweb_load_url(m_h, url.c_str()) != 0;
        }

        // 往页面推消息（JSON）
        bool emit(const std::wstring& json)
        {
            return m_h != nullptr && mfcweb_emit(m_h, json.c_str()) != 0;
        }

        // 向页面执行 JS
        bool eval(const std::wstring& js)
        {
            return m_h != nullptr && mfcweb_eval(m_h, js.c_str()) != 0;
        }

        // 无边框窗口的四边拖拽改尺寸：像素宽度，0 = 关闭；默认 6
        void setResizeBorder(int px)
        {
            if (m_h != nullptr) mfcweb_set_resize_border(m_h, px);
        }

        // 页面就绪后额外要收的一条消息（可选）
        void setReadyHandler(std::function<void()> handler)
        {
            m_ready = std::move(handler);
            if (m_h != nullptr)
                mfcweb_set_ready_cb(m_h, m_ready ? &WebShell::readyThunk : nullptr, this);
        }

        // 应用层命令钩子：收到页面消息时先问它，返回 true 表示已处理、不再走内置路由。
        void setCommandHandler(std::function<bool(const std::wstring& json)> handler)
        {
            m_command = std::move(handler);
            if (m_h != nullptr)
                mfcweb_set_command_cb(m_h, m_command ? &WebShell::commandThunk : nullptr, this);
        }

        // 宿主窗口（侧栏/主区那个子窗）
        HWND host() const { return m_h != nullptr ? mfcweb_host(m_h) : nullptr; }

        // WebView2 控制器是否已就绪（页面可交互）
        bool isReady() const { return m_h != nullptr && mfcweb_is_ready(m_h) != 0; }

        // 当前 DPI 缩放（页面 CSS px = DIP）
        double scale() const { return m_h != nullptr ? mfcweb_scale(m_h) : 1.0; }

        // 底层句柄（排查用；不要长期持有）
        mfcweb_shell handle() const { return m_h; }

    private:
        mfcweb_shell m_h = nullptr;
        std::function<void()> m_ready;
        std::function<bool(const std::wstring&)> m_command;

        static void readyThunk(void* user)
        {
            auto* self = static_cast<WebShell*>(user);
            if (self != nullptr && self->m_ready) self->m_ready();
        }

        static int commandThunk(void* user, const wchar_t* json)
        {
            auto* self = static_cast<WebShell*>(user);
            if (self == nullptr || !self->m_command) return 0;
            return self->m_command(std::wstring(json != nullptr ? json : L"")) ? 1 : 0;
        }

        WebShell(const WebShell&) = delete;
        WebShell& operator=(const WebShell&) = delete;
    };
}
