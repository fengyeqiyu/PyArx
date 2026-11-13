// quick_grab.cpp —— QQuickView 渲染验证：跑几秒后把窗口内容抓成 PNG，
// 用来判断"帧到底有没有画出来"。支持 QSG_RHI_BACKEND=software 环境变量。
#include <windows.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QUrl>
#include <QtGui/QGuiApplication>
#include <QtWidgets/QApplication>
#include <QtQuick/QQuickView>
#include <QtQuick/QQuickWindow>
#include <QtGui/QImage>
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv)
{
    FILE* log = std::fopen("G:\\PyArx\\docs\\_qtquick_grab.log", "a");
    QElapsedTimer t; t.start();
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);
    std::fprintf(log, "QApplication +%.1f ms\n", (double)t.nsecsElapsed()/1e6); std::fflush(log);

    int frames = 0;
    QQuickView v;
    v.resize(340, 620);
    v.setTitle("QQuickView");
    QObject::connect(&v, &QQuickWindow::frameSwapped, [&]() { ++frames; });
    v.setSource(QUrl::fromLocalFile("G:/PyArx/tests/qt_spike/qml_demo.qml"));
    std::fprintf(log, "setSource status=%d +%.1f ms\n", (int)v.status(), (double)t.nsecsElapsed()/1e6); std::fflush(log);
    v.show();
    std::fprintf(log, "show +%.1f ms\n", (double)t.nsecsElapsed()/1e6); std::fflush(log);

    QElapsedTimer run; run.start();
    while (run.elapsed() < 3000) {
        MSG msg;
        while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) { ::TranslateMessage(&msg); ::DispatchMessageW(&msg); }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        ::Sleep(5);
    }
    std::fprintf(log, "3 秒后 frames=%d +%.1f ms, renderer=%s\n", frames,
                 (double)t.nsecsElapsed()/1e6, v.rendererInterface() ? "有" : "无");
    QImage img = v.grabWindow();
    std::fprintf(log, "grabWindow %dx%d\n", img.width(), img.height()); std::fflush(log);
    std::fprintf(log, "save=%d\n", img.save("G:/PyArx/docs/_qtquick_grab.png") ? 1 : 0);
    std::fclose(log);
    return 0;
}
