// WebViewHost 实现：WebView2 环境 → Controller → CoreWebView2，含 DPI 折算与 JSON 收发。
#include "WebViewHost.h"

// WIN32_LEAN_AND_MEAN 不引入 ole2.h，而 WebView2.h 依赖其中的 interface 宏，必须先补上
#include <ole2.h>
#include <WebView2.h>
#include <wrl.h>
#include <wrl/implements.h>

#include <map>
#include <string>
#include <vector>
#include <stdio.h>

namespace webmfc
{
    namespace
    {
        // 路径 → file:// URL（UTF-8 百分号编码，反斜杠转正斜杠）
        std::wstring fileUrlOf(const std::wstring& path)
        {
            std::string utf8;
            const int len = ::WideCharToMultiByte(CP_UTF8, 0, path.c_str(), (int)path.size(), nullptr, 0,
                                                  nullptr, nullptr);
            if (len > 0)
            {
                utf8.resize((size_t)len);
                ::WideCharToMultiByte(CP_UTF8, 0, path.c_str(), (int)path.size(), utf8.data(), len, nullptr,
                                      nullptr);
            }

            std::string url = "file:///";
            for (char c : utf8)
            {
                const bool plain = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                                   c == '-' || c == '_' || c == '.' || c == '~' || c == '/' || c == ':' ||
                                   c == '\\';
                if (c == '\\') { url += '/'; continue; }
                if (plain) { url += c; continue; }
                char buf[8] = { 0 };
                sprintf_s(buf, "%%%02X", (unsigned char)c);
                url += buf;
            }
            return std::wstring(url.begin(), url.end());
        }

        // 默认数据目录：%LOCALAPPDATA%\PyArx\WebView2Data\<exe 名>
        //
        // 以前是 exe 同级 WebView2Data，结果每次跑 demo 都往 G:\PyArx\X64\Release
        // 里写 40+ MB 缓存（276 个文件），发布目录被当成了缓存目录。
        // 按 exe 名再分子目录是必须的：WebView2 的 user data 目录同一时刻只能被
        // 一个进程组占用，两个 exe 共用一个目录会互相把对方挤出浏览器进程。
        std::wstring defaultUserDataDir()
        {
            wchar_t exe[MAX_PATH] = { 0 };
            ::GetModuleFileNameW(nullptr, exe, MAX_PATH);
            std::wstring path(exe);

            std::wstring name = path.substr(path.find_last_of(L"\\/") + 1);
            const size_t dot = name.find_last_of(L'.');
            if (dot != std::wstring::npos)
                name.resize(dot);

            wchar_t base[MAX_PATH] = { 0 };
            if (::GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH) != 0 && base[0] != L'\0')
            {
                const std::wstring root = std::wstring(base) + L"\\PyArx";
                const std::wstring data = root + L"\\WebView2Data";
                const std::wstring dir = data + L"\\" + name;
                ::CreateDirectoryW(root.c_str(), nullptr);
                ::CreateDirectoryW(data.c_str(), nullptr);
                ::CreateDirectoryW(dir.c_str(), nullptr);
                return dir;
            }

            // 兜底：LOCALAPPDATA 拿不到（极少见）时退回 exe 同级，至少还能跑
            const size_t cut = path.find_last_of(L"\\/");
            if (cut != std::wstring::npos)
                path.resize(cut);
            return path + L"\\WebView2Data";
        }

        // ---- 阶段打点：回答"侧栏先出来，网页为什么慢两秒" ----
        LONGLONG qpcNow()
        {
            LARGE_INTEGER v;
            ::QueryPerformanceCounter(&v);
            return v.QuadPart;
        }

        LONGLONG qpcFreq()
        {
            static const LONGLONG f = []
            {
                LARGE_INTEGER v;
                ::QueryPerformanceFrequency(&v);
                return v.QuadPart;
            }();
            return f;
        }

        void phaseLog(const wchar_t* what, LONGLONG t0)
        {
            FILE* fp = ::_wfopen(L"G:\\PyArx\\docs\\_wv_timing.log", L"a");
            if (!fp)
                return;
            const double ms = (double)(qpcNow() - t0) * 1000.0 / (double)qpcFreq();
            SYSTEMTIME st;
            ::GetLocalTime(&st);
            ::fwprintf(fp, L"%02d:%02d:%02d.%03d  %-30s +%7.1f ms\n",
                       st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, what, ms);
            ::fclose(fp);
        }
    }

    struct WebViewHost::Impl
    {
        HWND parent = nullptr;
        bool creating = false;
        bool ready = false;
        bool visible = true;
        double scale = 1.0;
        LONGLONG t0 = 0;          // 阶段打点基准（QueryPerformanceCounter）
        RECT bounds{ 0, 0, 0, 0 };
        std::wstring pendingUrl;
        // WebView2 是异步建的：导航完成前 post 的消息会被丢进空白文档。
        // 这里先排队，等 NavigationCompleted 再一次性发出去。
        std::vector<std::wstring> pendingMsg;

        Microsoft::WRL::ComPtr<ICoreWebView2Environment> env;
        Microsoft::WRL::ComPtr<ICoreWebView2Controller> ctrl;
        Microsoft::WRL::ComPtr<ICoreWebView2> web;

        MessageHandler onMessage;
        std::function<void()> onReady;
        std::function<void(const std::wstring&)> onError;

        void fail(const std::wstring& text)
        {
            creating = false;
            if (onError) onError(text);
        }

        void applyBounds()
        {
            if (!ctrl) return;
            const double s = scale > 0.0 ? scale : 1.0;
            RECT dip = { (LONG)(bounds.left / s), (LONG)(bounds.top / s), (LONG)(bounds.right / s),
                         (LONG)(bounds.bottom / s) };
            ctrl->put_Bounds(dip);
        }
    };

    WebViewHost::WebViewHost() : m_impl(new Impl()) {}
    WebViewHost::~WebViewHost() { destroy(); }

    bool WebViewHost::create(HWND parent, const std::wstring& userDataDir, std::wstring& err)
    {
        err.clear();
        if (m_impl->creating || m_impl->ctrl)
        {
            err = L"WebView 已创建";
            return false;
        }
        if (!::IsWindow(parent))
        {
            err = L"宿主窗口无效";
            return false;
        }

        // WebView2 要求 STA；MFC 宿主通常已 AfxOleInit，这里兜底
        ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        m_impl->parent = parent;
        m_impl->creating = true;
        m_impl->t0 = qpcNow();
        phaseLog(L"attach 进入", m_impl->t0);

        double dpi = 96.0;
        {
            typedef UINT(WINAPI* GetDpiForWindowFn)(HWND);
            HMODULE u32 = ::GetModuleHandleW(L"user32.dll");
            if (u32)
            {
                auto fn = (GetDpiForWindowFn)::GetProcAddress(u32, "GetDpiForWindow");
                if (fn)
                {
                    UINT v = fn(parent);
                    if (v > 0) dpi = (double)v;
                }
            }
        }
        m_impl->scale = dpi / 96.0;

        const std::wstring dataDir = userDataDir.empty() ? defaultUserDataDir() : userDataDir;
        ::CreateDirectoryW(dataDir.c_str(), nullptr);

        const HRESULT hr = ::CreateCoreWebView2EnvironmentWithOptions(
            nullptr, dataDir.c_str(), nullptr,
            Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                    Impl* im = m_impl;
                    phaseLog(L"环境回调(env ready)", im->t0);
                    if (FAILED(result) || env == nullptr)
                    {
                        wchar_t buf[128] = { 0 };
                        swprintf_s(buf, L"WebView2 环境创建失败 hr=0x%08X", (unsigned)result);
                        im->fail(buf);
                        return S_OK;
                    }
                    im->env = env;
                    env->CreateCoreWebView2Controller(
                        im->parent,
                        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                            [im](HRESULT r2, ICoreWebView2Controller* c) -> HRESULT {
                                phaseLog(L"控制器回调(web ready)", im->t0);
                                if (FAILED(r2) || c == nullptr)
                                {
                                    wchar_t buf[128] = { 0 };
                                    swprintf_s(buf, L"WebView2 控制器创建失败 hr=0x%08X", (unsigned)r2);
                                    im->fail(buf);
                                    return S_OK;
                                }
                                im->ctrl = c;
                                c->get_CoreWebView2(&im->web);
                                im->creating = false;
                                if (!im->web) { im->fail(L"CoreWebView2 为空"); return S_OK; }

                                c->put_IsVisible(im->visible ? TRUE : FALSE);
                                im->applyBounds();

                                Microsoft::WRL::ComPtr<ICoreWebView2Settings> settings;
                                if (SUCCEEDED(im->web->get_Settings(&settings)) && settings)
                                {
                                    // 右键菜单交给页面自己做；开发者工具保留，调试期有用
                                    settings->put_AreDefaultContextMenusEnabled(FALSE);
                                    settings->put_IsStatusBarEnabled(FALSE);
                                    settings->put_AreDevToolsEnabled(TRUE);
                                }

                                // DPI：RasterizationScale 在 ICoreWebView2Controller3 上（1.0.4314 SDK 才有）
                                Microsoft::WRL::ComPtr<ICoreWebView2Controller3> c3;
                                if (SUCCEEDED(c->QueryInterface(IID_ICoreWebView2Controller3,
                                                                (void**)c3.GetAddressOf())) &&
                                    c3)
                                {
                                    c3->put_RasterizationScale(im->scale);
                                }

                                im->web->add_WebMessageReceived(
                                    Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                        [im](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args)
                                            -> HRESULT {
                                            if (!args) return S_OK;
                                            LPWSTR json = nullptr;
                                            if (SUCCEEDED(args->get_WebMessageAsJson(&json)) && json)
                                            {
                                                const std::wstring text(json);
                                                ::CoTaskMemFree(json);
                                                if (im->onMessage) im->onMessage(text);
                                            }
                                            return S_OK;
                                        })
                                        .Get(),
                                    nullptr);

                                im->web->add_NavigationCompleted(
                                    Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                        [im](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs* args)
                                            -> HRESULT {
                                            BOOL ok = TRUE;
                                            if (args) args->get_IsSuccess(&ok);
                                            im->ready = (ok == TRUE);
                                            phaseLog(ok == TRUE ? L"NavigationCompleted(页面就绪)"
                                                                   : L"NavigationCompleted(失败)",
                                                         im->t0);
                                            if (im->ready)
                                            {
                                                // 补发排队中的消息（页面此刻才真正挂好监听）
                                                if (im->web)
                                                {
                                                    for (const auto& m : im->pendingMsg)
                                                        im->web->PostWebMessageAsJson(m.c_str());
                                                }
                                                im->pendingMsg.clear();
                                                if (im->onReady) im->onReady();
                                            }
                                            return S_OK;
                                        })
                                        .Get(),
                                    nullptr);

                                if (!im->pendingUrl.empty())
                                {
                                    phaseLog(L"Navigate 发起(controller 建好)", im->t0);
                                    im->web->Navigate(im->pendingUrl.c_str());
                                    im->pendingUrl.clear();
                                }
                                phaseLog(L"controller 初始化完成", im->t0);
                                return S_OK;
                            })
                            .Get());
                    return S_OK;
                })
                .Get());

        phaseLog(L"CreateEnvironment 调用返回", m_impl->t0);

        if (FAILED(hr))
        {
            wchar_t buf[128] = { 0 };
            swprintf_s(buf, L"发起 WebView2 创建失败 hr=0x%08X", (unsigned)hr);
            err = buf;
            m_impl->creating = false;
            return false;
        }
        return true;
    }

    void WebViewHost::setBounds(const RECT& rcPixels)
    {
        m_impl->bounds = rcPixels;
        m_impl->applyBounds();
    }

    void WebViewHost::setVisible(bool visible)
    {
        m_impl->visible = visible;
        if (m_impl->ctrl) m_impl->ctrl->put_IsVisible(visible ? TRUE : FALSE);
    }

    bool WebViewHost::navigateFile(const std::wstring& htmlPath)
    {
        return navigateUrl(fileUrlOf(htmlPath));
    }

    bool WebViewHost::navigateUrl(const std::wstring& url)
    {
        if (!m_impl->web)
        {
            m_impl->pendingUrl = url;
            if (m_impl->t0) phaseLog(L"Navigate 挂起(web 未建好)", m_impl->t0);
            return true;
        }
        if (m_impl->t0) phaseLog(L"Navigate 发起(web 已就绪)", m_impl->t0);
        m_impl->web->Navigate(url.c_str());
        return true;
    }

    bool WebViewHost::postJson(const std::wstring& json)
    {
        if (!m_impl->web)
        {
            // 还没建好：先排队，等 NavigationCompleted 补发（最多留 32 条）
            if (m_impl->pendingMsg.size() < 32)
                m_impl->pendingMsg.push_back(json);
            return true;
        }
        const HRESULT hr = m_impl->web->PostWebMessageAsJson(json.c_str());
        if (FAILED(hr))
        {
            FILE* fp = ::_wfopen(L"G:\\PyArx\\docs\\_emit_dbg.log", L"a");
            if (fp)
            {
                ::fwprintf(fp, L"postJson FAILED hr=0x%08X json=%s\n", (unsigned)hr, json.c_str());
                ::fclose(fp);
            }
        }
        return SUCCEEDED(hr);
    }

    bool WebViewHost::eval(const std::wstring& js)
    {
        if (!m_impl->web) return false;
        return SUCCEEDED(m_impl->web->ExecuteScript(js.c_str(), nullptr));
    }

    void WebViewHost::setMessageHandler(MessageHandler handler) { m_impl->onMessage = std::move(handler); }
    void WebViewHost::setReadyHandler(std::function<void()> handler) { m_impl->onReady = std::move(handler); }
    void WebViewHost::setErrorHandler(std::function<void(const std::wstring&)> handler)
    {
        m_impl->onError = std::move(handler);
    }

    bool WebViewHost::isReady() const { return m_impl->ready; }
    double WebViewHost::scale() const { return m_impl->scale; }

    void WebViewHost::destroy()
    {
        if (!m_impl) return;
        if (m_impl->ctrl)
        {
            m_impl->ctrl->Close();
            m_impl->ctrl.Reset();
        }
        m_impl->web.Reset();
        m_impl->env.Reset();
        delete m_impl;
        m_impl = nullptr;
    }
}
