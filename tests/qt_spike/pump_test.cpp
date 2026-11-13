// 泵归属实验：不调用 app.exec()，改用"外部 Win32 消息泵 + 手动 processEvents"驱动 Qt，
// 模拟 CAD/MFC 的情形（主线程泵属于宿主，Qt 只是被客人调用）。
#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>
#include <QtWebEngineWidgets/QWebEngineView>
#include <windows.h>
#include <cstdio>

static QElapsedTimer g_t;
static FILE* g_log = nullptr;

static void mark(const char* what)
{
    const double ms = (double)g_t.nsecsElapsed() / 1000000.0;
    if (g_log) { std::fprintf(g_log, "%-40s +%8.1f ms\n", what, ms); std::fflush(g_log); }
    std::printf("[pump] %-40s +%8.1f ms\n", what, ms);
}

int main(int argc, char** argv)
{
    g_log = std::fopen("G:\\PyArx\\docs\\_qt_pump.log", "a");
    g_t.start();
    mark("进入 main");

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);       // 只构造，不 exec()
    mark("QApplication 构造完成");

    QWebEngineView* v = new QWebEngineView();
    v->resize(340, 700);
    v->show();
    mark("view show 返回");
    QObject::connect(v, &QWebEngineView::loadFinished, [](bool ok) {
        mark(ok ? "loadFinished(成功)" : "loadFinished(失败)");
    });
    v->load(QUrl::fromLocalFile(QString::fromUtf8("G:/PyArx/tests/cad/webview_status.html")));
    mark("load 发起");

    // 外部泵：Win32 消息 + 限时 processEvents，每轮睡 5 ms
    long long maxTick = 0;
    int ticks = 0;
    while (g_t.elapsed() < 6000) {
        MSG msg;
        while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
        QElapsedTimer tick;
        tick.start();
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        const long long d = tick.elapsed();
        if (d > maxTick) maxTick = d;
        ++ticks;
        ::Sleep(5);
    }
    if (g_log) {
        std::fprintf(g_log, "外部泵循环: %d 轮, 单轮 processEvents 最长 %lld ms\n", ticks, maxTick);
        std::fflush(g_log);
    }
    std::printf("[pump] 循环 %d 轮, 单轮最长 %lld ms\n", ticks, maxTick);

    v->grab().save("G:/PyArx/docs/_qt_pump_top.png");
    mark("抓图完成（外部泵驱动）");
    if (g_log) std::fclose(g_log);
    return 0;
}
