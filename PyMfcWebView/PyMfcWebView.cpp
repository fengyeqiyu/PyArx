// PyMfcWebView.cpp — 把 MFCWebView（静态库）暴露成 Python 扩展模块。
//
// 定位：与 PyMfcXaml 同级，**不依赖 CAD**，裸 python.exe 里 import 就能建窗。
// 窗口壳是 MFC（CFrameWnd / 或挂到已有 HWND），客户区整块交给 WebView2 渲染，
// 页面上所有控件都是 web 的；MFC 只负责窗口外壳与原生能力（拖拽/最小化/关闭/对话框）。
//
// 与 CAD 里的用法区别：
//   - 进程里没有 CWinApp（裸 python），这里自己补一个最小 CWinApp + AfxWinInit；
//   - 消息循环由 Python 侧驱动：run() 阻塞跑、runFor(ms) 跑一段、pump() 抽一帧，
//     方便脚本里自己控制生命周期，也方便自动化测试；
//   - 页面 → Python 的回调（setCommandHandler）在 UI 线程上执行，进回调前必须拿 GIL。
//
// Python 侧 API：
//   import PyMfcWebView as wv
//   w = wv.WebWindow(900, 640, "PyArx 窗体")          # 自带 MFC 框架窗
//   w.loadFile(r"G:\PyArx\tests\web\demo.html")
//   w.setCommandHandler(lambda json: True)            # 返回 True 表示已处理
//   w.emit('{"event":"py.hello"}')
//   w.run()                                           # 阻塞到窗口关闭
//   w.close()
//   w2 = wv.WebWindow.attachTo(hwnd, 340, 560)        # 挂到已有窗口（例如侧栏子窗）
//   w2.loadFile(...); w2.runFor(3000)

#include "stdafx.h"
#include "../MFCWebView/WebShell.h"
#include "../MFCWebView/JsonValue.h"

#include <algorithm>
#include <cstdio>
#include <objbase.h>
#include <string>

namespace
{
    // ---------------- 裸 Python 里的 MFC 自举 ----------------
    class PyWebStandaloneApp : public CWinApp
    {
    public:
        BOOL InitInstance() override { return TRUE; }
    };

    void ensureMfcApp()
    {
        // 裸 python 主线程默认没有 COM 套间（CAD 里是 AutoCAD 自己初始化好的），
        // 不初始化的话 WebView2 的 CreateAsync 会一直不回调，isReady() 永远 False。
        thread_local bool comDone = false;
        if (!comDone)
        {
            ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            comDone = true;
        }

        if (AfxGetApp() != nullptr)
            return;   // 宿主（CAD 之类）已提供 CWinApp

        static PyWebStandaloneApp* app = new PyWebStandaloneApp();
        UNREFERENCED_PARAMETER(app);
        AfxWinInit(::GetModuleHandleW(nullptr), nullptr, ::GetCommandLineW(), 0);
    }

    // WebView2 用户数据目录：裸 python 往往装在 Program Files 下，exe 同级不可写。
    // 一个 user data 目录同时只允许一个浏览器进程占用：CAD 里已经在用默认目录时，
    // 独立进程（裸 python / 演示 exe）再指过去就起不来浏览器进程，isReady 永远 False。
    // 所以允许用 PYARX_WEBVIEW_DATA 覆盖，独立会话自己挑一个目录。
    std::wstring webDataDir()
    {
        wchar_t override[MAX_PATH] = { 0 };
        if (::GetEnvironmentVariableW(L"PYARX_WEBVIEW_DATA", override, MAX_PATH) != 0 && override[0] != 0)
        {
            const std::wstring dir(override);
            ::CreateDirectoryW(dir.c_str(), nullptr);
            if (::GetFileAttributesW(dir.c_str()) != INVALID_FILE_ATTRIBUTES)
                return dir;
        }

        wchar_t base[MAX_PATH] = { 0 };
        if (::GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH) == 0 || base[0] == 0)
        {
            wchar_t tmp[MAX_PATH] = { 0 };
            ::GetTempPathW(MAX_PATH, tmp);
            wcscpy_s(base, MAX_PATH, tmp);
        }
        const std::wstring root = std::wstring(base) + L"\\PyArx";
        const std::wstring dir = root + L"\\WebView2Data";
        ::CreateDirectoryW(root.c_str(), nullptr);
        ::CreateDirectoryW(dir.c_str(), nullptr);
        return dir;
    }

    // 原生侧断点：pyd 里 MFC 抛的是 CException* 指针，pybind11 只能报"unknown exception"，
    // 所以关键步骤自己落一行日志，出事时能看出卡在哪一句。
    void logNative(const char* step)
    {
        const char* path = "G:\\PyArx\\docs\\_pywebview_native.log";
        FILE* f = std::fopen(path, "a");
        if (f == nullptr)
            return;
        std::fprintf(f, "%s\n", step);
        std::fclose(f);
    }

    // 页面 → Python 回调：UI 线程进 Python 必须拿 GIL，异常不许穿到消息循环
    std::string toUtf8(const std::wstring& text)    {
        if (text.empty())
            return std::string();
        const int len = ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                                              nullptr, 0, nullptr, nullptr);
        std::string out(static_cast<size_t>(len > 0 ? len : 0), '\0');
        if (len > 0)
            ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                                  &out[0], len, nullptr, nullptr);
        return out;
    }

    std::wstring fromUtf8(const char* text)
    {
        if (text == nullptr || text[0] == '\0')
            return std::wstring();
        const int len = ::MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
        std::wstring out(static_cast<size_t>(len > 0 ? len - 1 : 0), L'\0');
        if (len > 0)
            ::MultiByteToWideChar(CP_UTF8, 0, text, -1, &out[0], len);
        return out;
    }

    bool callPythonHandler(const py::object& fn, const std::wstring& json, bool* handled)
    {
        if (handled != nullptr)
            *handled = false;
        if (!fn || fn.is_none())
            return false;

        py::gil_scoped_acquire gil;
        try
        {
            py::object ret = fn(py::str(toUtf8(json)));
            const bool ok = !ret.is_none() && py::cast<bool>(ret);
            if (handled != nullptr)
                *handled = true;
            return ok;
        }
        catch (py::error_already_set& err)
        {
            std::wstring text = L"[PyMfcWebView] 回调抛异常：\n";
            text += fromUtf8(err.what());
            ::OutputDebugStringW(text.c_str());
            PyErr_Clear();
            if (handled != nullptr)
                *handled = true;   // 回调已接手，别再落到内置路由
            return true;
        }
    }
}

class PyWebWindow;

// 顶层窗口：裸 CWnd 当外壳（不用 CFrameWnd）。
// 不用框架窗的原因：CFrameWnd 自带工具条/停靠状态机，在 pyd 这种"无 CWinApp 宿主"里
// DestroyWindow 会抛未知异常、进程退出还会 0xC0000409；我们不需要框架能力，只要一个壳。
// PostNcDestroy 留空：窗口生命周期归 Python 对象管，不能让它自己 delete this。
class PyHostWnd : public CWnd
{
public:
    bool closed = false;
    PyWebWindow* owner = nullptr;

    // 窗口已经用 Win32 销毁时，把句柄摘掉，免得 ~CWnd 再去 DestroyWindow 一次
    void forgetHwnd() { m_hWnd = nullptr; }

protected:
    void PostNcDestroy() override {}
    afx_msg void OnClose();

    DECLARE_MESSAGE_MAP()
};

// 窗口对象：MFC 外壳窗（或挂已有 HWND）+ WebShell
class PyWebWindow
{
public:
    PyWebWindow() = default;
    ~PyWebWindow() { destroy(); }

    void markClosed() { m_closed = true; }

    bool create(int width, int height, const std::wstring& title, const std::wstring& html)
    {
        ensureMfcApp();
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        const CString cls = ::AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
                                                  ::LoadCursorW(nullptr, IDC_ARROW),
                                                  static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)), nullptr);
        CRect rc(0, 0, std::max(120, width), std::max(80, height));
        m_wnd = new PyHostWnd();
        m_wnd->owner = this;
        if (!m_wnd->CreateEx(WS_EX_APPWINDOW, cls, title.c_str(),
                             WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, rc, nullptr, 0))
        {
            delete m_wnd;
            m_wnd = nullptr;
            return false;
        }
        m_wnd->SetWindowTextW(title.c_str());

        if (!attachShell(m_wnd->GetSafeHwnd()))
            return false;

        if (!html.empty())
            loadFile(html);
        return true;
    }

    // 挂到已有窗口（侧栏子窗、宿主窗体都行）：客户区整块交给 WebView
    bool attachTo(unsigned long long hostHwnd)
    {
        ensureMfcApp();
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        if (!::IsWindow(reinterpret_cast<HWND>(hostHwnd)))
            return false;
        return attachShell(reinterpret_cast<HWND>(hostHwnd));
    }

    bool loadFile(const std::wstring& path)
    {
        if (m_shell == nullptr) return false;
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        return m_shell->loadPage(path);
    }

    bool loadUrl(const std::wstring& url)
    {
        if (m_shell == nullptr) return false;
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        return m_shell->loadUrl(url);
    }

    bool emit(const std::wstring& json)
    {
        if (m_shell == nullptr) return false;
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        return m_shell->emit(json);
    }

    bool eval(const std::wstring& js)
    {
        if (m_shell == nullptr) return false;
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        return m_shell->eval(js);
    }

    void show(int cmd = SW_SHOW)
    {
        if (m_wnd != nullptr)
            m_wnd->ShowWindow(cmd);
        else if (m_shell != nullptr && ::IsWindow(m_shell->host()))
            ::ShowWindow(m_shell->host(), cmd);
    }

    void hide()
    {
        if (m_wnd != nullptr)
            m_wnd->ShowWindow(SW_HIDE);
    }

    void resize(int width, int height)
    {
        if (m_wnd == nullptr)
            return;
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        m_wnd->SetWindowPos(nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
    }

    void center()
    {
        if (m_wnd == nullptr)
            return;
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        centerWindowOnScreen();
    }

    void setTitle(const std::wstring& title)
    {
        if (m_wnd == nullptr)
            return;
        m_wnd->SetWindowTextW(title.c_str());
    }

    // 阻塞跑到窗口关闭。返回 true 表示正常退出（窗口被关）。
    bool run()
    {
        MSG msg;
        while (!m_closed)
        {
            if (!::GetMessageW(&msg, nullptr, 0, 0))
                break;
            if (msg.message == WM_QUIT)
                break;
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
        return true;
    }

    // 跑 ms 毫秒就返回（自动化测试/嵌入别的循环时用）
    void runFor(int ms)
    {
        const DWORD deadline = ::GetTickCount() + static_cast<DWORD>(std::max(0, ms));
        MSG msg;
        while (::GetTickCount() < deadline)
        {
            while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                    return;
                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
            }
            ::Sleep(1);
        }
    }

    unsigned long long hwnd() const
    {
        if (m_wnd != nullptr)
            return reinterpret_cast<unsigned long long>(m_wnd->GetSafeHwnd());
        return m_shell != nullptr ? reinterpret_cast<unsigned long long>(m_shell->host()) : 0ull;
    }

    bool isReady() const { return m_shell != nullptr && m_shell->isReady(); }

    bool isClosed() const { return m_closed || m_wnd == nullptr; }

    void setCommandHandler(py::object fn)
    {
        m_commandHandler = std::move(fn);
        if (m_shell != nullptr)
        {
            m_shell->setCommandHandler([this](const std::wstring& json) -> bool
            {
                return callPythonHandler(m_commandHandler, json, nullptr);
            });
        }
    }

    void setReadyHandler(py::object fn)
    {
        m_readyHandler = std::move(fn);
        if (m_shell != nullptr)
        {
            m_shell->setReadyHandler([this]()
            {
                callPythonHandler(m_readyHandler, L"{\"event\":\"shell.ready\"}", nullptr);
            });
        }
    }

    void close()
    {
        m_closed = true;
        if (m_wnd != nullptr && ::IsWindow(m_wnd->GetSafeHwnd()))
            ::PostMessageW(m_wnd->GetSafeHwnd(), WM_CLOSE, 0, 0);
    }

    void destroy()
    {
        logNative("destroy: enter");
        if (m_shell != nullptr)
        {
            m_shell->detach();
            logNative("destroy: shell detached");
            delete m_shell;
            m_shell = nullptr;
            logNative("destroy: shell deleted");
        }
        if (m_wnd != nullptr)
        {
            // 用 Win32 的 DestroyWindow，不走 CWnd 包装；
            // 走包装时会因为临时句柄映射/框架残留抛 CException*（pybind11 只能报 unknown exception）。
            if (::IsWindow(m_wnd->GetSafeHwnd()))
            {
                ::DestroyWindow(m_wnd->GetSafeHwnd());
                logNative("destroy: window destroyed");
            }
            m_wnd->forgetHwnd();
            delete m_wnd;
            m_wnd = nullptr;
            logNative("destroy: window deleted");
        }
        logNative("destroy: leave");
    }

private:
    // 摆到主显示器正中
    void centerWindowOnScreen()
    {
        if (m_wnd == nullptr || !::IsWindow(m_wnd->GetSafeHwnd()))
            return;
        HMONITOR mon = ::MonitorFromWindow(m_wnd->GetSafeHwnd(), MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi{};
        mi.cbSize = sizeof(mi);
        if (!::GetMonitorInfoW(mon, &mi))
            return;
        RECT wr{};
        ::GetWindowRect(m_wnd->GetSafeHwnd(), &wr);
        const int w = wr.right - wr.left;
        const int h = wr.bottom - wr.top;
        const int x = mi.rcWork.left + ((mi.rcWork.right - mi.rcWork.left) - w) / 2;
        const int y = mi.rcWork.top + ((mi.rcWork.bottom - mi.rcWork.top) - h) / 2;
        ::SetWindowPos(m_wnd->GetSafeHwnd(), nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    bool attachShell(HWND host)
    {
        m_shell = new webmfc::WebShell();
        std::wstring err;
        if (!m_shell->attach(host, webDataDir(), err))
        {
            std::wstring text = L"[PyMfcWebView] WebView2 附加失败：" + err + L"\n";
            ::OutputDebugStringW(text.c_str());
            delete m_shell;
            m_shell = nullptr;
            return false;
        }

        m_shell->setCommandHandler([this](const std::wstring& json) -> bool
        {
            return callPythonHandler(m_commandHandler, json, nullptr);
        });
        m_shell->setReadyHandler([this]()
        {
            callPythonHandler(m_readyHandler, L"{\"event\":\"shell.ready\"}", nullptr);
        });

        // 页面画自己的标题条时才有意义；这里给默认 6px，无边框窗口也能拖边改尺寸
        m_shell->setResizeBorder(6);
        return true;
    }

    PyHostWnd* m_wnd = nullptr;
    webmfc::WebShell* m_shell = nullptr;
    py::object m_commandHandler;
    py::object m_readyHandler;
    bool m_closed = false;
};

void PyHostWnd::OnClose()
{
    closed = true;
    if (owner != nullptr)
        owner->markClosed();
    CWnd::OnClose();
}

BEGIN_MESSAGE_MAP(PyHostWnd, CWnd)
    ON_WM_CLOSE()
END_MESSAGE_MAP()

PYBIND11_MODULE(PyMfcWebView, m)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    m.doc() = "PyMfcWebView — MFC 窗体 + WebView2 的 Python 接口（页面控件全是 web，MFC 只管外壳）";

    py::class_<PyWebWindow>(m, "WebWindow")
        .def(py::init([](int width, int height, const std::string& title, const std::string& html)
        {
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            auto win = std::make_unique<PyWebWindow>();
            if (!win->create(width, height, fromUtf8(title.c_str()), fromUtf8(html.c_str())))
                throw std::runtime_error("建窗失败：检查 WebView2 运行时是否安装");
            return win;
        }),
        py::arg("width") = 900, py::arg("height") = 640,
        py::arg("title") = "PyArx 窗体", py::arg("html") = "",
        "建一个带 MFC 框架的窗口，客户区整块给 WebView；html 传本地文件路径则立即加载")
        .def("attachTo", [](PyWebWindow& self, unsigned long long hostHwnd)
        {
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            return self.attachTo(hostHwnd);
        }, py::arg("hostHwnd"), "挂到已有窗口句柄（侧栏子窗等），不新建框架窗")
        .def("loadFile", [](PyWebWindow& self, const std::string& path)
        {
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            return self.loadFile(fromUtf8(path.c_str()));
        }, py::arg("path"), "加载本地单文件 HTML")
        .def("loadUrl", [](PyWebWindow& self, const std::string& url)
        {
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            return self.loadUrl(fromUtf8(url.c_str()));
        }, py::arg("url"), "加载网址")
        .def("emit", [](PyWebWindow& self, const std::string& json)
        {
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            return self.emit(fromUtf8(json.c_str()));
        }, py::arg("json"), "往页面推一条 JSON（页面用 message 事件收）")
        .def("eval", [](PyWebWindow& self, const std::string& js)
        {
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            return self.eval(fromUtf8(js.c_str()));
        }, py::arg("js"), "在页面里执行一段 JS")
        .def("setCommandHandler", [](PyWebWindow& self, py::object fn)
        {
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            self.setCommandHandler(std::move(fn));
        }, py::arg("fn"), "页面消息回调：fn(json_str) -> bool，返回 True 表示已处理、不再走内置路由")
        .def("setReadyHandler", [](PyWebWindow& self, py::object fn)
        {
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            self.setReadyHandler(std::move(fn));
        }, py::arg("fn"), "WebView 就绪回调：fn(json_str)")
        .def("show", &PyWebWindow::show, py::arg("cmd") = SW_SHOW)
        .def("hide", &PyWebWindow::hide)
        .def("resize", &PyWebWindow::resize, py::arg("width"), py::arg("height"))
        .def("center", &PyWebWindow::center)
        .def("setTitle", [](PyWebWindow& self, const std::string& title)
        {
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            self.setTitle(fromUtf8(title.c_str()));
        }, py::arg("title"))
        .def("run", &PyWebWindow::run, "阻塞跑消息循环，直到窗口被关")
        .def("runFor", &PyWebWindow::runFor, py::arg("ms"), "跑 ms 毫秒消息循环后返回")
        .def("close", &PyWebWindow::close)
        .def("destroy", &PyWebWindow::destroy)
        .def("hwnd", &PyWebWindow::hwnd, "窗口句柄（int）")
        .def("isReady", &PyWebWindow::isReady, "WebView2 是否已完成初始化")
        .def("isClosed", &PyWebWindow::isClosed);

    // 一步到位的入口：建窗 + 加载页面 + 挂页面回调 + 抽消息直到 WebView 就绪。
    // 页面控件（按钮/输入框）全部走 web，MFC 只当外壳；页面消息直接落进 Python 回调。
    m.def("webLoad", [](const std::string& page, int width, int height, const std::string& title,
                        py::object onCommand, py::object onReady, int waitReadyMs)
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        auto win = std::make_unique<PyWebWindow>();
        if (!win->create(width, height, fromUtf8(title.c_str()), fromUtf8(page.c_str())))
            throw std::runtime_error("建窗失败：检查 WebView2 运行时是否安装");

        if (!onCommand.is_none())
            win->setCommandHandler(std::move(onCommand));
        if (!onReady.is_none())
            win->setReadyHandler(std::move(onReady));

        win->center();
        win->show();

        // 等 WebView 就绪：返回时页面已经能 postMessage，调用方接着 run()/runFor() 即可
        const DWORD deadline = ::GetTickCount() + static_cast<DWORD>(std::max(0, waitReadyMs));
        while (!win->isReady() && ::GetTickCount() < deadline)
            win->runFor(20);

        return win;
    }, py::arg("page"), py::arg("width") = 900, py::arg("height") = 640,
       py::arg("title") = "PyArx 窗体",
       py::arg("onCommand") = py::none(), py::arg("onReady") = py::none(),
       py::arg("waitReadyMs") = 10000,
       "建窗 + 加载 page（本地 HTML 路径或网址）+ 挂 onCommand/onReady，等到 WebView 就绪后返回 WebWindow");

    // 更懒的入口：加载页面后直接阻塞跑消息循环，窗口关掉才返回
    m.def("webRun", [](const std::string& page, int width, int height, const std::string& title,
                       py::object onCommand, py::object onReady)
    {
        auto window = std::make_unique<PyWebWindow>();
        if (!window->create(width, height, fromUtf8(title.c_str()), fromUtf8(page.c_str())))
            throw std::runtime_error("建窗失败：检查 WebView2 运行时是否安装");
        if (!onCommand.is_none())
            window->setCommandHandler(std::move(onCommand));
        if (!onReady.is_none())
            window->setReadyHandler(std::move(onReady));
        window->center();
        window->show();

        const DWORD deadline = ::GetTickCount() + 10000;
        while (!window->isReady() && ::GetTickCount() < deadline)
            window->runFor(20);
        window->run();          // 阻塞到窗口关闭
        window->destroy();
        return true;
    }, py::arg("page"), py::arg("width") = 900, py::arg("height") = 640,
       py::arg("title") = "PyArx 窗体",
       py::arg("onCommand") = py::none(), py::arg("onReady") = py::none(),
       "加载页面并阻塞跑消息循环，窗口关闭后返回");

    m.def("runFor", [](int ms)
    {
        MSG msg;
        const DWORD deadline = ::GetTickCount() + static_cast<DWORD>(std::max(0, ms));
        while (::GetTickCount() < deadline)
        {
            while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT) return;
                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
            }
            ::Sleep(1);
        }
    }, py::arg("ms") = 0, "不挂在窗口上时，单独抽一会儿消息");

    m.def("parseJson", [](const std::string& text)
    {
        webmfc::JsonValue v;
        std::wstring err;
        if (!webmfc::parseJson(fromUtf8(text.c_str()), v, &err))
            throw std::runtime_error("JSON 解析失败：" + toUtf8(err));
        return text;
    }, py::arg("text"), "用内置 JSON 解析器校验一段文本（失败抛异常）");
}
