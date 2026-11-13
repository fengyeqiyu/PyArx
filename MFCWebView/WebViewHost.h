#pragma once
// WebViewHost：把 Edge WebView2 嵌进任意宿主窗口，对外只暴露"导航 / 发消息 / 收消息 / 改尺寸"。
// 设计约束：本文件不暴露任何 WebView2 或 WRL 的 COM 细节，调用方只需要 windows.h。
#include <windows.h>
#include <functional>
#include <string>

namespace webmfc
{
    class WebViewHost
    {
    public:
        // HTML → C++：收到的是原始 JSON 文本（页面用 chrome.webview.postMessage 发出）
        using MessageHandler = std::function<void(const std::wstring& json)>;

        WebViewHost();
        ~WebViewHost();

        // 创建（异步）。返回 false 只表示"发起失败"，真正可用以 readyHandler 为准。
        // userDataDir：WebView2 用户数据目录，传空则用 exe 同级的 WebView2Data
        bool create(HWND parent, const std::wstring& userDataDir, std::wstring& err);

        // 尺寸按父窗口客户区像素传入；内部按 DPI 折算成 DIP
        void setBounds(const RECT& rcPixels);
        void setVisible(bool visible);

        // 打开本地单文件 HTML（内部转成 file:// URL）
        bool navigateFile(const std::wstring& htmlPath);
        bool navigateUrl(const std::wstring& url);

        // 往页面发 JSON（页面用 chrome.webview.addEventListener('message') 收）
        bool postJson(const std::wstring& json);
        // 往页面执行 JS
        bool eval(const std::wstring& js);

        void setMessageHandler(MessageHandler handler);
        void setReadyHandler(std::function<void()> handler);
        void setErrorHandler(std::function<void(const std::wstring&)> handler);

        bool isReady() const;
        // 当前 DPI 缩放（页面 CSS px = DIP；原生控件用像素时按它乘）
        double scale() const;

        void destroy();

    private:
        struct Impl;
        Impl* m_impl = nullptr;

        WebViewHost(const WebViewHost&) = delete;
        WebViewHost& operator=(const WebViewHost&) = delete;
    };
}
