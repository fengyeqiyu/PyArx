// quick_test.cpp —— 独立 exe 验证 Qt Quick 的两种承载方式：
//   widget: QQuickWidget（QWidget 容器，走 FBO 渲染）
//   view  : QQuickView（原生 QWindow，可直接 SetParent）
// 不调 app.exec()，用外部 PeekMessage 泵，模拟 CAD 内的工作方式。
#include <windows.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QUrl>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtQuickWidgets/QQuickWidget>
#include <QtQuick/QQuickView>
#include <QtQuick/QQuickWindow>
#include <cstdio>
#include <cstring>

static FILE* g_log;
static QElapsedTimer g_t;

static void L(const char* fmt, ...)
{
    SYSTEMTIME st; ::GetLocalTime(&st);
    std::fprintf(g_log, "%02d:%02d:%02d.%03d  ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    va_list ap; va_start(ap, fmt); std::vfprintf(g_log, fmt, ap); va_end(ap);
    std::fprintf(g_log, "\n"); std::fflush(g_log);
}
static double ms() { return (double)g_t.nsecsElapsed() / 1e6; }

int main(int argc, char** argv)
{
    const char* mode = argc > 1 ? argv[1] : "widget";
    const char* qml = argc > 2 ? argv[2] : "G:/PyArx/tests/qt_spike/qml_demo.qml";
    g_log = std::fopen("G:\\PyArx\\docs\\_qtquick_standalone.log", "a");
    L("===== mode=%s qml=%s =====", mode, qml);

    g_t.start();
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);
    L("QApplication 构造完成 +%.1f ms", ms());

    if (std::strcmp(mode, "widget") == 0) {
        QQuickWidget w;
        L("QQuickWidget 构造完成 +%.1f ms", ms());
        w.setResizeMode(QQuickWidget::SizeRootObjectToView);
        w.resize(340, 620);
        QObject::connect(w.quickWindow(), &QQuickWindow::frameSwapped, [](void) { L("帧交换 +%.1f ms", ms()); });
        w.setSource(QUrl::fromLocalFile(qml));
        L("setSource 完成 +%.1f ms status=%d", ms(), (int)w.status());
        L("show 之前 +%.1f ms", ms());
        w.show();
        L("show 返回 +%.1f ms", ms());
        w.setWindowTitle("QQuickWidget");
    } else {
        QQuickView v;
        L("QQuickView 构造完成 +%.1f ms", ms());
        v.resize(340, 620);
        QObject::connect(&v, &QQuickWindow::frameSwapped, [](void) { L("帧交换 +%.1f ms", ms()); });
        v.setSource(QUrl::fromLocalFile(qml));
        L("setSource 完成 +%.1f ms status=%d", ms(), (int)v.status());
        v.show();
        L("show 返回 +%.1f ms", ms());
        v.setTitle("QQuickView");
    }
    L("进入外部泵 +%.1f ms", ms());

    int rounds = 0;
    long long maxRound = 0;
    QElapsedTimer endAll; endAll.start();
    while (endAll.elapsed() < 8000) {
        MSG msg;
        while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg); ::DispatchMessageW(&msg);
        }
        QElapsedTimer rt; rt.start();
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        const long long d = rt.elapsed();
        if (d > maxRound) maxRound = d;
        ++rounds;
        ::Sleep(5);
    }
    L("泵结束 轮数=%d 单轮最长=%lld ms 总 %.1f ms", rounds, maxRound, ms());
    std::fclose(g_log);
    return 0;
}
