// QtSpike.pyd —— 在 AutoCAD 进程内把 QtWebEngine 跑起来（真机验证，不是独立 exe）。
//
// 做的事：
//   1. 用 CAD 目录里的 Qt 6.2.6 DLL（本 pyd 用 6.2.4 SDK 链接，跨版本 ABI 实测）；
//   2. 自建 QApplication（先设 AA_ShareOpenGLContexts），并在 CAD 主线程上装一个 Win32 定时器
//      驱动 QCoreApplication::processEvents —— 即"宿主泵 + 客人 Qt"，不抢 CAD 的消息循环；
//   3. 把 QWebEngineView 加载本地 HTML：parent HWND 为 0 时开独立窗口，
//      否则用 QWindow::fromWinId + createWindowContainer 塞进指定窗口。
#include <Python.h>
#include "windows.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtGui/QWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWebEngineWidgets/QWebEngineView>
#include <cstdarg>
#include <cstdio>
#include <string>

static FILE* g_log = nullptr;
static QElapsedTimer g_timer;
static QApplication* g_app = nullptr;
static QWidget* g_container = nullptr;
static QWebEngineView* g_view = nullptr;
static HWND g_parent = nullptr;
static HWND g_pumpWnd = nullptr;
static long long g_maxTick = 0;
static int g_ticks = 0;
static int g_grabDone = 0;
static std::wstring g_outDir;

static void logf(const char* fmt, ...)
{
    if (!g_log) return;
    SYSTEMTIME st;
    ::GetLocalTime(&st);
    std::fprintf(g_log, "%02d:%02d:%02d.%03d  ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(g_log, fmt, ap);
    va_end(ap);
    std::fprintf(g_log, "\n");
    std::fflush(g_log);
}

static double sinceStart()
{
    return g_timer.isValid() ? (double)g_timer.nsecsElapsed() / 1000000.0 : 0.0;
}

// 每轮只给 Qt 5 ms，绝不霸占 CAD 的泵
static void pumpQt()
{
    if (!g_app) return;
    QElapsedTimer tick;
    tick.start();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    const long long d = tick.elapsed();
    if (d > g_maxTick) g_maxTick = d;
    ++g_ticks;

    // 跟随父窗口尺寸
    if (g_parent && g_view && ::IsWindow(g_parent)) {
        RECT rc;
        ::GetClientRect(g_parent, &rc);
        const int w = (int)(rc.right - rc.left);
        const int h = (int)(rc.bottom - rc.top);
        if (w > 0 && h > 0) {
            if (g_container) g_container->resize(w, h);
            g_view->resize(w, h);
        }
    }
}

static LRESULT CALLBACK PumpProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_TIMER && wp == 1) {
        pumpQt();
        return 0;
    }
    return ::DefWindowProcW(h, msg, wp, lp);
}

static int ensurePumpWindow()
{
    if (g_pumpWnd) return 1;
    static const wchar_t* kClass = L"PyArxQtSpikePump";
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = PumpProc;
    wc.hInstance = ::GetModuleHandleW(nullptr);
    wc.lpszClassName = kClass;
    if (!::RegisterClassExW(&wc)) {
        logf("RegisterClassExW 失败 err=%lu", ::GetLastError());
        return 0;
    }
    g_pumpWnd = ::CreateWindowExW(0, kClass, L"", 0, 0, 0, 0, 0,
                                 HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
    if (!g_pumpWnd) {
        logf("CreateWindowExW(message-only) 失败 err=%lu", ::GetLastError());
        return 0;
    }
    if (!::SetTimer(g_pumpWnd, 1, 16, nullptr)) {
        logf("SetTimer 失败 err=%lu", ::GetLastError());
        return 0;
    }
    logf("泵定时器已装（16 ms）");
    return 1;
}

static bool ensureQt()
{
    if (g_app) {
        logf("QApplication 已存在，复用");
        return true;
    }
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    // 指向 CAD 自带的 WebEngine 资源 / 进程 / QPA 插件
    wchar_t exe[MAX_PATH] = {};
    ::GetModuleFileNameW(nullptr, exe, MAX_PATH);
    std::wstring dir(exe);
    dir = dir.substr(0, dir.find_last_of(L"\\/"));
    std::wstring res = dir + L"\\resources";
    std::wstring loc = dir + L"\\translations\\qtwebengine_locales";
    std::wstring proc = dir + L"\\QtWebEngineProcess.exe";
    qputenv("QTWEBENGINE_RESOURCES_PATH", QDir::toNativeSeparators(QString::fromWCharArray(res.c_str())).toUtf8());
    qputenv("QTWEBENGINE_LOCALES_PATH", QDir::toNativeSeparators(QString::fromWCharArray(loc.c_str())).toUtf8());
    qputenv("QTWEBENGINEPROCESS_PATH", QDir::toNativeSeparators(QString::fromWCharArray(proc.c_str())).toUtf8());
    qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
            QDir::toNativeSeparators(QString::fromWCharArray((dir + L"\\Plugins\\platforms").c_str())).toUtf8());
    logf("资源根目录 = %ls", dir.c_str());

    static int argc = 1;
    static char arg0[] = "acad";
    static char* argv[] = { arg0, nullptr };
    g_timer.start();
    g_app = new QApplication(argc, argv);
    g_app->setQuitOnLastWindowClosed(false);   // 关掉 Qt 窗口不许影响宿主
    logf("QApplication 构造完成 +%.1f ms", sinceStart());
    return g_app != nullptr;
}

static PyObject* py_run(PyObject*, PyObject* args)
{
    const char* html = nullptr;
    unsigned long long parent = 0;
    int w = 340, h = 600;
    if (!PyArg_ParseTuple(args, "sKii", &html, &parent, &w, &h)) return nullptr;

    if (!g_log) {
        g_log = std::fopen("G:\\PyArx\\docs\\_qt_in_cad.log", "a");
    }
    logf("===== 进入 run(html=%s, parent=0x%llX, %dx%d) =====", html, parent, w, h);

    if (!ensureQt()) { Py_RETURN_FALSE; }
    if (!ensurePumpWindow()) { Py_RETURN_FALSE; }

    g_parent = (HWND)parent;
    if (g_parent && ::IsWindow(g_parent)) {
        RECT rc = {};
        ::GetClientRect(g_parent, &rc);
        logf("嵌进外来窗口 0x%llX 客户区 %dx%d", parent, (int)(rc.right - rc.left), (int)(rc.bottom - rc.top));
        // 经典托管：先让 Qt 建出自己的顶层窗口，再用 Win32 SetParent 把它挂进 MFC 页窗口，
        // 并把 WS_POPUP 改成 WS_CHILD。比 createWindowContainer 更"土"，但不会把宿主窗口抽走。
        g_view = new QWebEngineView();
        g_view->resize(w, h);
        g_view->show();
        HWND qtHwnd = (HWND)g_view->winId();
        logf("Qt 顶层窗口 hwnd = 0x%llX", (unsigned long long)qtHwnd);
        ::SetParent(qtHwnd, g_parent);
        LONG_PTR style = ::GetWindowLongPtrW(qtHwnd, GWL_STYLE);
        style = (style & ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME)) | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        ::SetWindowLongPtrW(qtHwnd, GWL_STYLE, style);
        ::SetWindowPos(qtHwnd, HWND_TOP, 0, 0, w, h, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        logf("SetParent 完成 +%.1f ms", sinceStart());
        g_view->show();
        logf("嵌入后 view 就绪 +%.1f ms", sinceStart());
    } else {
        logf("独立窗口形态");
        g_view = new QWebEngineView();
        g_view->resize(w, h);
        g_view->move(120, 120);
        g_view->show();
        logf("view show 返回 +%.1f ms", sinceStart());
    }

    QObject::connect(g_view, &QWebEngineView::loadFinished, [](bool ok) {
        logf(ok ? "loadFinished(成功) +%.1f ms" : "loadFinished(失败) +%.1f ms", sinceStart());
    });
    g_view->load(QUrl::fromLocalFile(QString::fromUtf8(html)));
    logf("load 发起 +%.1f ms", sinceStart());

    Py_RETURN_TRUE;
}

static PyObject* py_stats(PyObject*, PyObject*)
{
    return Py_BuildValue("{s:i,s:L,s:i,s:i}",
                        "ticks", g_ticks,
                        "max_tick_ms", g_maxTick,
                        "has_app", g_app ? 1 : 0,
                        "has_view", g_view ? 1 : 0);
}

static PyObject* py_grab(PyObject*, PyObject*)
{
    if (!g_view) Py_RETURN_FALSE;
    pumpQt();
    const QString path = QString::fromWCharArray(g_outDir.c_str()) + "/_qt_in_cad_view.png";
    const bool ok = g_view->grab().save(path);
    logf("抓图 %s -> %s", ok ? "成功" : "失败", path.toUtf8().constData());
    return PyBool_FromLong(ok ? 1 : 0);
}

static PyObject* py_stop(PyObject*, PyObject*)
{
    if (g_pumpWnd) { ::KillTimer(g_pumpWnd, 1); logf("泵定时器已停"); }
    Py_RETURN_NONE;
}

static PyMethodDef g_methods[] = {
    { "run",   py_run,   METH_VARARGS, "run(html, parent_hwnd, w, h)" },
    { "stats", py_stats, METH_NOARGS,  "stats()" },
    { "grab",  py_grab,  METH_NOARGS,  "grab()" },
    { "stop",  py_stop,  METH_NOARGS,  "stop()" },
    { nullptr, nullptr, 0, nullptr }
};

static PyModuleDef g_module = {
    PyModuleDef_HEAD_INIT, "QtSpike", "AutoCAD 内 QtWebEngine spike", -1, g_methods
};

PyMODINIT_FUNC PyInit_QtSpike(void)
{
    g_outDir = L"G:\\PyArx\\docs";
    return PyModule_Create(&g_module);
}
