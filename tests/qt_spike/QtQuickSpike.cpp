// QtQuickSpike.pyd —— 在 AutoCAD 进程里用 Qt Quick(QML) 画侧栏界面。
//
// 与 WebEngine 版共用同一套思路：自建 QApplication（AA_ShareOpenGLContexts）、
// 宿主泵驱动 processEvents、SetParent 把 QQuickWidget 挂进 CAdUiPalette 页窗口。
#include <Python.h>
#include "windows.h"
#include "QtBackend.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QUrl>
#include <QtGui/QWindow>
#include <QtWidgets/QApplication>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickView>
#include <QtQml/QQmlContext>
#include <cstdarg>
#include <cstdio>
#include <string>

static FILE* g_log = nullptr;
static QElapsedTimer g_timer;
static QApplication* g_app = nullptr;
static QQuickView* g_view = nullptr;
static QtBackend* g_backend = nullptr;
static PyObject* g_cb = nullptr;
static HWND g_parent = nullptr;
static HWND g_pumpWnd = nullptr;
static long long g_maxTick = 0;
static int g_ticks = 0;
static int g_frames = 0;

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

// 给 QtBackend.cpp 用的两个入口：回调指针 + 写日志
PyObject* qt_backend_callback() { return g_cb; }
void qt_backend_log(const char* msg) { logf("%s", msg); }

static void pumpQt()
{
    if (!g_app) return;
    QElapsedTimer tick;
    tick.start();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    const long long d = tick.elapsed();
    if (d > g_maxTick) g_maxTick = d;
    ++g_ticks;

    if (g_parent && g_view && ::IsWindow(g_parent)) {
        RECT rc;
        ::GetClientRect(g_parent, &rc);
        const int w = (int)(rc.right - rc.left);
        const int h = (int)(rc.bottom - rc.top);
        if (w > 0 && h > 0 && (g_view->width() != w || g_view->height() != h))
            g_view->resize(w, h);
    }
}

static LRESULT CALLBACK PumpProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_TIMER && wp == 1) { pumpQt(); return 0; }
    return ::DefWindowProcW(h, msg, wp, lp);
}

static bool ensurePumpWindow()
{
    if (g_pumpWnd) return 1;
    static const wchar_t* kClass = L"PyArxQtQuickPump";
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = PumpProc;
    wc.hInstance = ::GetModuleHandleW(nullptr);
    wc.lpszClassName = kClass;
    if (!::RegisterClassExW(&wc)) { logf("RegisterClassExW 失败 %lu", ::GetLastError()); return 0; }
    g_pumpWnd = ::CreateWindowExW(0, kClass, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
    if (!g_pumpWnd) { logf("CreateWindowExW 失败 %lu", ::GetLastError()); return 0; }
    if (!::SetTimer(g_pumpWnd, 1, 16, nullptr)) { logf("SetTimer 失败 %lu", ::GetLastError()); return 0; }
    logf("泵定时器已装（16 ms）");
    return 1;
}

static bool ensureQt()
{
    if (g_app) { logf("QApplication 已存在，复用"); return true; }
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    wchar_t exe[MAX_PATH] = {};
    ::GetModuleFileNameW(nullptr, exe, MAX_PATH);
    std::wstring dir(exe);
    dir = dir.substr(0, dir.find_last_of(L"\\/"));
    qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
            QDir::toNativeSeparators(QString::fromWCharArray((dir + L"\\Plugins\\platforms").c_str())).toUtf8());

    // QML 模块目录：CAD 自己没有 qml\，这里先指向 SDK 的（后续部署才是正式做法）
    qputenv("QML2_IMPORT_PATH",
            QDir::toNativeSeparators(QString::fromUtf8("G:/Qt/6.2.4/msvc2019_64/qml")).toUtf8());

    static int argc = 1;
    static char arg0[] = "acad";
    static char* argv[] = { arg0, nullptr };
    g_timer.start();
    g_app = new QApplication(argc, argv);
    g_app->setQuitOnLastWindowClosed(false);
    logf("QApplication 构造完成 +%.1f ms", sinceStart());
    return true;
}

// QML 插件依赖的 Qt DLL：CAD 只带了 20 个 Qt6 DLL，缺 Qt6QmlWorkerScript 这类，
// 不预载就会在 import QtQuick 时报"找不到指定的模块"。
static void preloadQmlDeps(const std::wstring& srcDir)
{
    static const wchar_t* kDeps[] = {
        L"Qt6QmlWorkerScript.dll", L"Qt6QmlLocalStorage.dll", L"Qt6QmlXmlListModel.dll",
        L"Qt6QmlCore.dll", L"Qt6QuickControls2.dll", L"Qt6QuickControls2Impl.dll",
        L"Qt6QuickTemplates2.dll", L"Qt6QuickLayouts.dll", L"Qt6QuickShapes.dll",
        L"Qt6QuickParticles.dll", L"Qt6OpenGLWidgets.dll", L"Qt6SvgWidgets.dll",
        nullptr
    };
    for (int i = 0; kDeps[i]; ++i) {
        if (::GetModuleHandleW(kDeps[i])) { logf("已有 %ls", kDeps[i]); continue; }
        const std::wstring p = srcDir + L"\\" + kDeps[i];
        const HMODULE h = ::LoadLibraryW(p.c_str());
        logf("预载 %ls -> %s", kDeps[i], h ? "成功" : "失败");
    }
}

static PyObject* py_run_qml(PyObject*, PyObject* args)
{
    const char* qml = nullptr;
    unsigned long long parent = 0;
    int w = 340, h = 620;
    if (!PyArg_ParseTuple(args, "sKii", &qml, &parent, &w, &h)) return nullptr;

    if (!g_log) g_log = std::fopen("G:\\PyArx\\docs\\_qtquick_in_cad.log", "a");
    logf("===== run_qml(%s, parent=0x%llX, %dx%d) =====", qml, parent, w, h);

    if (!ensureQt()) Py_RETURN_FALSE;
    if (!ensurePumpWindow()) Py_RETURN_FALSE;
    preloadQmlDeps(L"G:\\Qt\\6.2.4\\msvc2019_64\\bin");

    g_parent = (HWND)parent;
    // 用 QQuickView（原生 QWindow）而不是 QQuickWidget：后者在本机 show() 会卡死。
    g_view = new QQuickView();
    logf("QQuickView 构造完成 +%.1f ms", sinceStart());
    g_view->setResizeMode(QQuickView::SizeRootObjectToView);
    g_view->resize(w, h);

    // 把 C++ 后端对象注入 QML，QML 里直接 backend.addNumbers(...)
    if (!g_backend) g_backend = new QtBackend();
    g_view->rootContext()->setContextProperty("backend", g_backend);

    QObject::connect(g_view, &QQuickWindow::frameSwapped, [](void) {
        ++g_frames;
        if (g_frames <= 3 || g_frames == 10)
            logf("第 %d 帧已交换 +%.1f ms", g_frames, sinceStart());
    });

    g_view->setSource(QUrl::fromLocalFile(QString::fromUtf8(qml)));
    logf("setSource 完成 +%.1f ms, status=%d", sinceStart(), (int)g_view->status());
    if (g_view->status() == QQuickView::Error) {
        for (const auto& e : g_view->errors())
            logf("QML 错误: %s", e.toString().toUtf8().constData());
        Py_RETURN_FALSE;
    }

    g_view->show();
    logf("show 返回 +%.1f ms", sinceStart());

    if (g_parent && ::IsWindow(g_parent)) {
        RECT rc = {};
        ::GetClientRect(g_parent, &rc);
        logf("宿主客户区 %dx%d", (int)(rc.right - rc.left), (int)(rc.bottom - rc.top));
        HWND qtHwnd = (HWND)g_view->winId();
        logf("QQuickView 窗口 0x%llX", (unsigned long long)qtHwnd);
        ::SetParent(qtHwnd, g_parent);
        LONG_PTR style = ::GetWindowLongPtrW(qtHwnd, GWL_STYLE);
        style = (style & ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME)) | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        ::SetWindowLongPtrW(qtHwnd, GWL_STYLE, style);
        ::SetWindowPos(qtHwnd, HWND_TOP, 0, 0, w, h, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        logf("SetParent 完成 +%.1f ms", sinceStart());
    }
    Py_RETURN_TRUE;
}

static PyObject* py_stats(PyObject*, PyObject*){
    return Py_BuildValue("{s:i,s:L,s:i,s:i,s:i}",
                        "ticks", g_ticks, "max_tick_ms", g_maxTick,
                        "frames", g_frames, "has_app", g_app ? 1 : 0, "has_view", g_view ? 1 : 0);
}

static PyObject* py_grab(PyObject*, PyObject*)
{
    if (!g_view) Py_RETURN_FALSE;
    pumpQt();
    const bool ok = g_view->grabWindow().save("G:/PyArx/docs/_qtquick_in_cad.png");
    logf("抓图 %s", ok ? "成功" : "失败");
    return PyBool_FromLong(ok ? 1 : 0);
}

static PyObject* py_set_callback(PyObject*, PyObject* args)
{
    PyObject* fn = nullptr;
    if (!PyArg_ParseTuple(args, "O", &fn)) return nullptr;
    Py_XINCREF(fn);
    Py_XDECREF(g_cb);
    g_cb = fn;
    logf("Python 回调已注册");
    Py_RETURN_TRUE;
}

static PyObject* py_push(PyObject*, PyObject* args)
{
    const char* text = nullptr;
    if (!PyArg_ParseTuple(args, "s", &text)) return nullptr;
    if (!g_backend) Py_RETURN_FALSE;
    g_backend->setStatus(QString::fromUtf8(text));   // Python -> QML 方向
    logf("Python 推送到 QML: %s", text);
    Py_RETURN_TRUE;
}

static PyObject* py_query(PyObject*, PyObject* args)
{
    const char* name = nullptr;
    if (!PyArg_ParseTuple(args, "s", &name)) return nullptr;
    if (!g_backend) Py_RETURN_NONE;
    const QByteArray prop(name);
    const QVariant v = g_backend->property(prop.constData());
    if (!v.isValid()) Py_RETURN_NONE;
    return PyUnicode_FromString(v.toString().toUtf8().constData());
}

static PyObject* py_hwnd(PyObject*, PyObject*)
{
    if (!g_view) return PyLong_FromLongLong(0);
    return PyLong_FromLongLong((long long)(unsigned long long)g_view->winId());
}

static PyObject* py_stop(PyObject*, PyObject*)
{
    if (g_pumpWnd) { ::KillTimer(g_pumpWnd, 1); logf("泵定时器已停"); }
    Py_RETURN_NONE;
}

static PyMethodDef g_methods[] = {
    { "run_qml", py_run_qml, METH_VARARGS, "run_qml(qml_path, parent_hwnd, w, h)" },
    { "stats",   py_stats,   METH_NOARGS,  "stats()" },
    { "grab",    py_grab,    METH_NOARGS,  "grab()" },
    { "stop",    py_stop,    METH_NOARGS,  "stop()" },
    { "set_callback", py_set_callback, METH_VARARGS, "set_callback(fn)" },
    { "push",    py_push,    METH_VARARGS,  "push(text)" },
    { "query",   py_query,   METH_VARARGS,  "query(prop_name)" },
    { "hwnd",    py_hwnd,    METH_NOARGS,   "hwnd()" },
    { nullptr, nullptr, 0, nullptr }
};

static PyModuleDef g_module = {
    PyModuleDef_HEAD_INIT, "QtQuickSpike", "AutoCAD 内 Qt Quick spike", -1, g_methods
};

PyMODINIT_FUNC PyInit_QtQuickSpike(void)
{
    return PyModule_Create(&g_module);
}
