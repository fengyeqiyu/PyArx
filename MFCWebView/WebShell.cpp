// WebShell 实现：宿主窗口子类化 + 客户区铺满 WebView + JSON 命令路由 + 原生能力。
#include "WebShellCore.h"
#include "MFCWebViewApi.h"
#include "JsonValue.h"

#include <commctrl.h>
#include <commdlg.h>
#include <windowsx.h>

#include <map>
#include <sstream>
#include <string>
#include <new>
#include <cstring>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

namespace webmfc
{
    namespace
    {
        // ---------------- 命令读取门面（底层是真正的 JSON 解析） ----------------
        // 保留原来的 f.str / f.i / f.d / f.b 调用形状，换成按路径取值；
        // 页面传多行代码、带引号的文本、反斜杠路径都不会再被转义糊掉。
        struct JsonFlat
        {
            JsonValue root;
            bool valid = false;

            // 参数取法：先按顶层键取，取不到再落到 args.<键>。
            // 页面里 Python 那侧惯用 {cmd, args:{...}}，内置路由原先只认顶层键，
            // 两种写法都认之后，页面不用为同一条消息记两套规矩。
            const JsonValue* pick(const wchar_t* key) const
            {
                if (!valid || key == nullptr)
                    return nullptr;
                const JsonValue* v = root.findPath(key);
                if (v != nullptr)
                    return v;
                return root.findPath(std::wstring(L"args.") + key);
            }
            bool has(const wchar_t* key) const { return pick(key) != nullptr; }
            std::wstring str(const wchar_t* key, const wchar_t* dflt = L"") const
            {
                const JsonValue* v = pick(key);
                return v != nullptr ? v->asString(dflt) : std::wstring(dflt);
            }
            int i(const wchar_t* key, int dflt = 0) const
            {
                const JsonValue* v = pick(key);
                return v != nullptr ? v->asInt(dflt) : dflt;
            }
            double d(const wchar_t* key, double dflt = 0.0) const
            {
                const JsonValue* v = pick(key);
                return v != nullptr ? v->asNumber(dflt) : dflt;
            }
            bool b(const wchar_t* key, bool dflt = false) const
            {
                const JsonValue* v = pick(key);
                return v != nullptr ? v->asBool(dflt) : dflt;
            }
            const JsonValue* at(const wchar_t* path) const { return valid ? root.findPath(path) : nullptr; }
        };

        bool parseFlat(const std::wstring& text, JsonFlat& out)
        {
            out.valid = parseJson(text, out.root, nullptr);
            return out.valid;
        }

        std::wstring jsonEscape(const std::wstring& s) { return escapeJson(s); }

        std::wstring evt(const std::wstring& name)
        {
            return L"{\"event\":\"" + jsonEscape(name) + L"\"";
        }

        UINT dpiOf(HWND hwnd)
        {
            typedef UINT(WINAPI* GetDpiForWindowFn)(HWND);
            HMODULE u32 = ::GetModuleHandleW(L"user32.dll");
            if (u32)
            {
                auto fn = (GetDpiForWindowFn)::GetProcAddress(u32, "GetDpiForWindow");
                if (fn)
                {
                    UINT v = fn(hwnd);
                    if (v > 0) return v;
                }
            }
            return 96;
        }

        bool isLightTheme()
        {
            HKEY key = nullptr;
            if (::RegOpenKeyExW(HKEY_CURRENT_USER,
                                L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0,
                                KEY_READ, &key) != ERROR_SUCCESS)
                return true;
            DWORD v = 1, size = sizeof(v), type = 0;
            const bool ok = ::RegQueryValueExW(key, L"AppsUseLightTheme", nullptr, &type,
                                               (LPBYTE)&v, &size) == ERROR_SUCCESS;
            ::RegCloseKey(key);
            return ok ? (v != 0) : true;
        }

        std::wstring stateName(HWND root)
        {
            if (!::IsWindowVisible(root)) return L"hidden";
            if (::IsZoomed(root)) return L"max";
            if (::IsIconic(root)) return L"min";
            return L"normal";
        }
    }

    struct Shell::Impl
    {
        HWND host = nullptr;
        HWND root = nullptr;
        WebViewHost view;
        int resizeBorder = 6;
        std::function<void()> onReady;
        std::function<bool(const std::wstring&)> onCommand;
        std::wstring pendingPage;

        void fill()
        {
            RECT rc{};
            ::GetClientRect(host, &rc);
            view.setBounds(rc);
        }

        void emitJson(const std::wstring& json) { view.postJson(json); }

        void emitWindowState()
        {
            HWND root = ::IsWindow(this->root) ? this->root : this->host;
            RECT rc{};
            ::GetClientRect(this->host, &rc);
            emitJson(evt(L"window.state") + L",\"state\":\"" + stateName(root) +
                     L"\",\"width\":" + std::to_wstring(rc.right - rc.left) +
                     L",\"height\":" + std::to_wstring(rc.bottom - rc.top) + L"}");
        }

        void onMessage(const std::wstring& json);
        static LRESULT CALLBACK sub(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    };

    Shell::Shell() : m_impl(new Impl()) {}
    Shell::~Shell() { detach(); delete m_impl; }

    bool Shell::attach(HWND host, const std::wstring& userDataDir, std::wstring& err)
    {
        err.clear();
        if (!::IsWindow(host))
        {
            err = L"宿主窗口无效";
            return false;
        }
        m_impl->host = host;
        m_impl->root = ::GetAncestor(host, GA_ROOT);

        // 窗口尺寸变化时自动铺满（宿主是子窗也能吃 WM_SIZE）
        ::SetWindowSubclass(host, &Impl::sub, 0x4D46, (DWORD_PTR)m_impl);

        m_impl->view.setMessageHandler([this](const std::wstring& j) { m_impl->onMessage(j); });
        m_impl->view.setReadyHandler([this]() {
            // 控制器是异步建出来的：attach 时宿主往往已经有尺寸、WM_SIZE 也早就走过了，
            // 那时 ctrl 还不存在，setBounds 是空转。就绪后必须自己补一次铺满，
            // 否则 WebView 子窗停在 0x0（CAD 侧栏里表现为一片空白）。
            m_impl->fill();
            RECT rc{};
            ::GetClientRect(m_impl->host, &rc);
            const UINT dpi = dpiOf(m_impl->host);
            wchar_t buf[256] = { 0 };
            swprintf_s(buf, L"%.2f", dpi / 96.0);
            // dpi 报真实 DPI（96/120/144…），缩放比单独放 scale——以前把缩放比塞在 dpi 里，
            // 页面按 dpi 判断"是不是高分屏"永远得到 1。
            std::wstring e = evt(L"shell.ready") + L",\"dpi\":" + std::to_wstring(dpi) +
                             L",\"scale\":" + std::wstring(buf) +
                             L",\"theme\":\"" + (isLightTheme() ? L"light" : L"dark") + L"\"" +
                             L",\"width\":" + std::to_wstring(rc.right - rc.left) +
                             L",\"height\":" + std::to_wstring(rc.bottom - rc.top) +
                             L",\"state\":\"" + stateName(m_impl->root) + L"\"}";
            m_impl->emitJson(e);
            if (m_impl->onReady) m_impl->onReady();
        });
        m_impl->view.setErrorHandler([this](const std::wstring& t) {
            m_impl->emitJson(evt(L"shell.error") + L",\"text\":\"" + jsonEscape(t) + L"\"}");
        });

        if (!m_impl->view.create(host, userDataDir, err))
            return false;

        m_impl->fill();
        return true;
    }

    void Shell::detach()
    {
        if (!m_impl) return;
        if (::IsWindow(m_impl->host))
            ::RemoveWindowSubclass(m_impl->host, &Impl::sub, 0x4D46);
        m_impl->view.destroy();
        m_impl->host = nullptr;
        m_impl->root = nullptr;
    }

    bool Shell::loadPage(const std::wstring& htmlPath) { return m_impl->view.navigateFile(htmlPath); }
    bool Shell::loadUrl(const std::wstring& url) { return m_impl->view.navigateUrl(url); }
    bool Shell::emit(const std::wstring& json) { return m_impl->view.postJson(json); }
    bool Shell::eval(const std::wstring& js) { return m_impl->view.eval(js); }
    void Shell::setResizeBorder(int px) { m_impl->resizeBorder = px; }
    void Shell::setReadyHandler(std::function<void()> handler) { m_impl->onReady = std::move(handler); }
    void Shell::setCommandHandler(std::function<bool(const std::wstring&)> handler)
    {
        m_impl->onCommand = std::move(handler);
    }
    HWND Shell::host() const { return m_impl->host; }
    WebViewHost& Shell::view() { return m_impl->view; }

    // ======================= 宿主窗口子类化 =======================
    LRESULT CALLBACK Shell::Impl::sub(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR ref)
    {
        Impl* self = (Impl*)ref;
        if (self == nullptr) return ::DefSubclassProc(hwnd, msg, wp, lp);

        switch (msg)
        {
        case WM_SIZE:
            if (self->view.isReady() || self->host == hwnd) self->fill();
            {
                RECT rc{};
                ::GetClientRect(hwnd, &rc);
                HWND root = ::IsWindow(self->root) ? self->root : hwnd;
                self->emitJson(evt(L"window.resized") + L",\"width\":" + std::to_wstring(rc.right - rc.left) +
                               L",\"height\":" + std::to_wstring(rc.bottom - rc.top) +
                               L",\"state\":\"" + stateName(root) + L"\"}");
            }
            break;

        case WM_DPICHANGED:
        {
            RECT rc{};
            ::GetClientRect(hwnd, &rc);
            self->view.setBounds(rc);
            break;
        }

        // 无边框窗体：让四边可以被拖动改尺寸（页面不用画一个假边框去模拟）
        case WM_NCHITTEST:
        {
            if (self->resizeBorder > 0 && ::IsWindow(self->root) && !::IsZoomed(self->root))
            {
                RECT wr{};
                ::GetWindowRect(self->root, &wr);
                POINT pt{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
                const int b = self->resizeBorder;
                const bool l = pt.x >= wr.left && pt.x < wr.left + b;
                const bool r = pt.x <= wr.right && pt.x > wr.right - b;
                const bool t = pt.y >= wr.top && pt.y < wr.top + b;
                const bool bt = pt.y <= wr.bottom && pt.y > wr.bottom - b;
                if (t && l) return HTTOPLEFT;
                if (t && r) return HTTOPRIGHT;
                if (bt && l) return HTBOTTOMLEFT;
                if (bt && r) return HTBOTTOMRIGHT;
                if (l) return HTLEFT;
                if (r) return HTRIGHT;
                if (t) return HTTOP;
                if (bt) return HTBOTTOM;
            }
            break;
        }
        }
        return ::DefSubclassProc(hwnd, msg, wp, lp);
    }

    // ======================= 命令路由 =======================
    void Shell::Impl::onMessage(const std::wstring& json)
    {
        JsonFlat f;
        if (!parseFlat(json, f)) return;
        const std::wstring cmd = f.str(L"cmd");
        if (cmd.empty()) return;

        // 先给应用层一次接管机会（多实例协作命令，如 nav / pane.*）
        if (this->onCommand && this->onCommand(json)) return;

        HWND root = ::IsWindow(this->root) ? this->root : this->host;

        if (cmd == L"shell.info")
        {
            RECT rc{};
            ::GetClientRect(this->host, &rc);
            const UINT dpi = dpiOf(this->host);
            wchar_t scaleBuf[32] = { 0 };
            swprintf_s(scaleBuf, L"%.2f", dpi / 96.0);
            emitJson(evt(L"shell.info") + L",\"dpi\":" + std::to_wstring(dpi) +
                     L",\"scale\":" + scaleBuf +
                     L",\"theme\":\"" + (isLightTheme() ? L"light" : L"dark") + L"\"" +
                     L",\"width\":" + std::to_wstring(rc.right - rc.left) +
                     L",\"height\":" + std::to_wstring(rc.bottom - rc.top) +
                     L",\"state\":\"" + stateName(root) + L"\"}");
            return;
        }

        if (cmd == L"window.title")
        {
            ::SetWindowTextW(root, f.str(L"text").c_str());
            return;
        }

        if (cmd == L"window.drag")
        {
            // 页面自绘标题条按下时调用：把拖动交给系统，窗口就跟手走
            ::ReleaseCapture();
            ::SendMessageW(root, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            return;
        }

        if (cmd == L"window.min") { ::ShowWindow(root, SW_MINIMIZE); emitWindowState(); return; }
        if (cmd == L"window.max") { ::ShowWindow(root, SW_MAXIMIZE); emitWindowState(); return; }
        if (cmd == L"window.restore") { ::ShowWindow(root, SW_RESTORE); emitWindowState(); return; }
        if (cmd == L"window.close") { ::PostMessageW(root, WM_CLOSE, 0, 0); return; }

        if (cmd == L"window.resize")
        {
            const int w = f.i(L"w", 0), h = f.i(L"h", 0);
            if (w > 0 && h > 0)
            {
                RECT rc{ 0, 0, w, h };
                ::AdjustWindowRectEx(&rc, (DWORD)::GetWindowLongPtrW(root, GWL_STYLE), FALSE,
                                     (DWORD)::GetWindowLongPtrW(root, GWL_EXSTYLE));
                ::SetWindowPos(root, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
                               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            emitWindowState();
            return;
        }

        if (cmd == L"window.center")
        {
            RECT wr{}, work{};
            ::GetWindowRect(root, &wr);
            MONITORINFO mi{ sizeof(mi) };
            if (::GetMonitorInfoW(::MonitorFromWindow(root, MONITOR_DEFAULTTONEAREST), &mi))
            {
                work = mi.rcWork;
                const int w = wr.right - wr.left, h = wr.bottom - wr.top;
                const int x = work.left + ((work.right - work.left) - w) / 2;
                const int y = work.top + ((work.bottom - work.top) - h) / 2;
                ::SetWindowPos(root, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            return;
        }

        if (cmd == L"dialog.open" || cmd == L"dialog.save")
        {
            std::wstring filter = f.str(L"filter", L"所有文件 (*.*)|*.*||");
            // 页面给的是 "XAML|*.xaml" 这种简写，转成 Win32 的双分隔格式
            if (filter.find(L'|') != std::wstring::npos && filter.find(L"||") == std::wstring::npos)
            {
                const size_t p = filter.find(L'|');
                std::wstring left = filter.substr(0, p), right = filter.substr(p + 1);
                filter = left + L" (" + right + L")|" + right + L"|";
            }
            wchar_t file[MAX_PATH] = { 0 };
            const std::wstring defName = f.str(L"name");
            if (!defName.empty()) wcsncpy_s(file, defName.c_str(), _TRUNCATE);

            OPENFILENAMEW ofn{};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = root;
            ofn.lpstrFilter = filter.c_str();
            ofn.lpstrFile = file;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrTitle = f.str(L"title", L"选择文件").c_str();
            ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY |
                        (cmd == L"dialog.save" ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);

            const BOOL ok = (cmd == L"dialog.save") ? ::GetSaveFileNameW(&ofn) : ::GetOpenFileNameW(&ofn);
            std::wstring resp = evt(cmd) + L",\"id\":\"" + jsonEscape(f.str(L"id")) + L"\"";
            resp += L",\"canceled\":";
            resp += (ok ? L"false" : L"true");
            resp += L",\"path\":\"" + jsonEscape(ok ? std::wstring(file) : std::wstring()) + L"\"}";
            emitJson(resp);
            return;
        }

        if (cmd == L"dialog.message")
        {
            UINT flags = MB_OK | MB_ICONINFORMATION;
            const std::wstring level = f.str(L"level", L"info");
            if (level == L"warn") flags = MB_OK | MB_ICONWARNING;
            else if (level == L"error") flags = MB_OK | MB_ICONERROR;
            ::MessageBoxW(root, f.str(L"text").c_str(), f.str(L"title", L"提示").c_str(), flags);
            emitJson(evt(L"dialog.message") + L",\"id\":\"" + jsonEscape(f.str(L"id")) + L"\"}");
            return;
        }

        if (cmd == L"clipboard.set")
        {
            const std::wstring text = f.str(L"text");
            bool ok = false;
            if (::OpenClipboard(this->host))
            {
                ::EmptyClipboard();
                const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
                if (HGLOBAL h = ::GlobalAlloc(GMEM_MOVEABLE, bytes))
                {
                    if (void* p = ::GlobalLock(h))
                    {
                        memcpy(p, text.c_str(), bytes);
                        ::GlobalUnlock(h);
                        ok = ::SetClipboardData(CF_UNICODETEXT, h) != nullptr;
                    }
                    if (!ok) ::GlobalFree(h);
                }
                ::CloseClipboard();
            }
            emitJson(evt(L"clipboard.set") + L",\"ok\":" + (ok ? L"true" : L"false") + L"}");
            return;
        }

        if (cmd == L"clipboard.get")
        {
            std::wstring text;
            if (::OpenClipboard(this->host))
            {
                if (HANDLE h = ::GetClipboardData(CF_UNICODETEXT))
                {
                    if (const wchar_t* p = (const wchar_t*)::GlobalLock(h))
                    {
                        text = p;
                        ::GlobalUnlock(h);
                    }
                }
                ::CloseClipboard();
            }
            emitJson(evt(L"clipboard.get") + L",\"id\":\"" + jsonEscape(f.str(L"id")) +
                     L"\",\"text\":\"" + jsonEscape(text) + L"\"}");
            return;
        }

        if (cmd == L"shell.log")
        {
            ::OutputDebugStringW((L"[web] " + f.str(L"text") + L"\n").c_str());
            return;
        }
    }

    namespace
    {
        void writeErr(wchar_t* err, int cch, const std::wstring& text)
        {
            if (err == nullptr || cch <= 0) return;
            const size_t n = text.size() < static_cast<size_t>(cch - 1)
                                 ? text.size()
                                 : static_cast<size_t>(cch - 1);
            ::memcpy(err, text.c_str(), n * sizeof(wchar_t));
            err[n] = L'\0';
        }
    }
}

// ============================ C ABI 导出（DLL 门面） ============================
// 只做参数搬运：C 字符串 / 句柄进出，C++ 侧的对象一律不越过 DLL 边界。
extern "C"
{
    MFCWEB_API mfcweb_shell mfcweb_create(void)
    {
        return new (std::nothrow) webmfc::Shell();
    }

    MFCWEB_API void mfcweb_destroy(mfcweb_shell s)
    {
        delete static_cast<webmfc::Shell*>(s);
    }

    MFCWEB_API int mfcweb_attach(mfcweb_shell s, HWND host, const wchar_t* userDataDir,
                                 wchar_t* err, int errCch)
    {
        auto* shell = static_cast<webmfc::Shell*>(s);
        if (shell == nullptr) return 0;
        std::wstring reason;
        const bool ok = shell->attach(host, userDataDir != nullptr ? userDataDir : L"", reason);
        if (!ok) webmfc::writeErr(err, errCch, reason);
        return ok ? 1 : 0;
    }

    MFCWEB_API void mfcweb_detach(mfcweb_shell s)
    {
        if (auto* shell = static_cast<webmfc::Shell*>(s)) shell->detach();
    }

    MFCWEB_API int mfcweb_load_page(mfcweb_shell s, const wchar_t* htmlPath)
    {
        auto* shell = static_cast<webmfc::Shell*>(s);
        return (shell != nullptr && htmlPath != nullptr && shell->loadPage(htmlPath)) ? 1 : 0;
    }

    MFCWEB_API int mfcweb_load_url(mfcweb_shell s, const wchar_t* url)
    {
        auto* shell = static_cast<webmfc::Shell*>(s);
        return (shell != nullptr && url != nullptr && shell->loadUrl(url)) ? 1 : 0;
    }

    MFCWEB_API int mfcweb_emit(mfcweb_shell s, const wchar_t* json)
    {
        auto* shell = static_cast<webmfc::Shell*>(s);
        return (shell != nullptr && json != nullptr && shell->emit(json)) ? 1 : 0;
    }

    MFCWEB_API int mfcweb_eval(mfcweb_shell s, const wchar_t* js)
    {
        auto* shell = static_cast<webmfc::Shell*>(s);
        return (shell != nullptr && js != nullptr && shell->eval(js)) ? 1 : 0;
    }

    MFCWEB_API void mfcweb_set_resize_border(mfcweb_shell s, int px)
    {
        if (auto* shell = static_cast<webmfc::Shell*>(s)) shell->setResizeBorder(px);
    }

    MFCWEB_API void mfcweb_set_ready_cb(mfcweb_shell s, mfcweb_ready_fn fn, void* user)
    {
        auto* shell = static_cast<webmfc::Shell*>(s);
        if (shell == nullptr) return;
        if (fn == nullptr)
            shell->setReadyHandler(nullptr);
        else
            shell->setReadyHandler([fn, user]() { fn(user); });
    }

    MFCWEB_API void mfcweb_set_command_cb(mfcweb_shell s, mfcweb_command_fn fn, void* user)
    {
        auto* shell = static_cast<webmfc::Shell*>(s);
        if (shell == nullptr) return;
        if (fn == nullptr)
            shell->setCommandHandler(nullptr);
        else
            shell->setCommandHandler([fn, user](const std::wstring& json) {
                return fn(user, json.c_str()) != 0;
            });
    }

    MFCWEB_API HWND mfcweb_host(mfcweb_shell s)
    {
        auto* shell = static_cast<webmfc::Shell*>(s);
        return shell != nullptr ? shell->host() : nullptr;
    }

    MFCWEB_API int mfcweb_is_ready(mfcweb_shell s)
    {
        auto* shell = static_cast<webmfc::Shell*>(s);
        return (shell != nullptr && shell->view().isReady()) ? 1 : 0;
    }

    MFCWEB_API double mfcweb_scale(mfcweb_shell s)
    {
        auto* shell = static_cast<webmfc::Shell*>(s);
        return shell != nullptr ? shell->view().scale() : 1.0;
    }

    MFCWEB_API const wchar_t* mfcweb_version(void)
    {
        return L"MFCWebView 1.0 (DLL) / WebView2";
    }
}
