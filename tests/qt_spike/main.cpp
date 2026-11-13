// QtWebEngine spike：CAD 自带 Qt 6.2.6 能不能当界面用，以及两次初始化各花多久。
// 两个形态都测：
//   A) 独立顶层窗口（QWebEngineView 自己建窗）
//   B) 嵌进"外来 HWND"（模拟 MFC/CAD 的页窗口：QWindow::fromWinId + createWindowContainer）
#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtGui/QWindow>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWebEngineWidgets/QWebEngineView>
#include <windows.h>
#include <cstdio>

static QElapsedTimer g_timer;
static FILE* g_log = nullptr;

static void mark(const char* what)
{
    const double ms = g_timer.isValid() ? (double)g_timer.nsecsElapsed() / 1000000.0 : 0.0;
    if (g_log)
    {
        SYSTEMTIME st;
        ::GetLocalTime(&st);
        std::fprintf(g_log, "%02d:%02d:%02d.%03d  %-34s +%8.1f ms\n",
                     st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, what, ms);
        std::fflush(g_log);
    }
    std::printf("[qt_spike] %-34s +%8.1f ms\n", what, ms);
}

int main(int argc, char** argv)
{
    g_log = std::fopen("G:\\PyArx\\docs\\_qt_spike.log", "a");
    g_timer.start();
    mark("进程启动(main 进入)");

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    mark("QApplication 构造前");
    QApplication app(argc, argv);
    mark("QApplication 构造完成");

    const QString html = QString::fromUtf8("G:/PyArx/tests/cad/webview_status.html");

    // ---------- A) 独立顶层窗口 ----------
    mark("A: QWebEngineView 构造前");
    QWebEngineView* v1 = new QWebEngineView();
    mark("A: QWebEngineView 构造完成");
    v1->resize(360, 700);
    v1->move(80, 80);
    v1->show();
    mark("A: show 返回");
    QObject::connect(v1, &QWebEngineView::loadFinished, [](bool ok) {
        mark(ok ? "A: loadFinished(成功)" : "A: loadFinished(失败)");
    });
    v1->load(QUrl::fromLocalFile(html));
    mark("A: load 发起");

    QTimer::singleShot(1500, [v1]() {
        mark("A: 抓图");
        v1->grab().save("G:/PyArx/docs/_qt_spike_top.png");
        mark("A: 抓图完成");
    });

    // ---------- B) 嵌进外来 HWND（模拟 MFC 页窗口） ----------
    QTimer::singleShot(2200, []() {
        mark("B: 建 Win32 宿主窗");
        HWND host = ::CreateWindowExW(0, L"STATIC", L"qt spike host",
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                      500, 80, 380, 720,
                                      nullptr, nullptr, nullptr, nullptr);
        mark("B: 宿主窗建好");
        QWindow* w = QWindow::fromWinId((WId)host);
        mark("B: QWindow::fromWinId");
        QWidget* container = QWidget::createWindowContainer(w);
        mark("B: createWindowContainer");
        container->resize(360, 680);
        container->show();
        QWebEngineView* v2 = new QWebEngineView(container);
        v2->resize(360, 680);
        v2->show();
        mark("B: 子 view 就绪，准备 load");
        v2->load(QUrl::fromLocalFile(QString::fromUtf8("G:/PyArx/tests/cad/webview_status.html")));
        mark("B: load 发起");
        QObject::connect(v2, &QWebEngineView::loadFinished, [](bool ok) {
            mark(ok ? "B: loadFinished(成功)" : "B: loadFinished(失败)");
        });
        QTimer::singleShot(1500, [container]() {
            container->grab().save("G:/PyArx/docs/_qt_spike_embed.png");
            mark("B: 抓图完成");
        });
    });

    QTimer::singleShot(6000, [&app]() { mark("退出"); app.quit(); });
    const int rc = app.exec();
    if (g_log) std::fclose(g_log);
    return rc;
}
