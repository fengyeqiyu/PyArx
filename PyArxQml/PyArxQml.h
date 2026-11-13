// PyArxQml.h —— QML 面板宿主（在 AutoCAD 进程内跑 Qt Quick，并把窗口托管进侧栏）。
#pragma once
#include <string>

namespace pyarx {
namespace qml {

struct HostStats
{
    int ticks = 0;              // 泵轮次
    long long maxTickMs = 0;    // 单轮最长耗时
    int frames = 0;             // 已交换帧数
    bool hasApp = false;
    bool hasView = false;
};

class QmlHost
{
public:
    static QmlHost& Instance();

    // 首次调用建 QApplication（约 3 秒，进程内只付一次）并装泵定时器
    bool Ensure(bool preloadDeps = true);

    // 探针专用：只建 QCoreApplication（不建 GUI、不装泵），用于二分"Qt 核心初始化"与"平台/字体初始化"。
    // 注意：调用后本进程不能再建 QGuiApplication，用完必须退出进程。
    bool CoreOnlyProbe();

    // 渲染 qmlFile 到新的 QQuickView，再 SetParent 进 parentHwnd
    bool Attach(const std::wstring& qmlFile, void* parentHwnd, int width, int height);

    void Pump();
    void Stop();
    void* Hwnd() const;
    HostStats Stats() const;
    // 场景图后端：0=Unknown 1=Software 2=OpenGL 3=D3D11 4=Vulkan 5=Metal 6=Null
    int RenderBackend() const;
    bool Grab(const std::wstring& pngPath) const;

    // Python -> QML：改注入对象的 status 属性
    bool PushStatus(const std::string& text);
    // 读注入对象的属性值（用于脚本校验界面文本）
    std::string QueryProp(const std::string& name);

    // 供 Python 侧定位 QML 对象（返回 QObject*；未加载返回 nullptr）
    // RootObject() 是 qml 根对象，FindItem(name) 按 objectName 递归查找，name 为空时等价于根。
    void* RootObject() const;
    void* FindItem(const std::string& objectName) const;
    // 供 eval(expr) 用：QQmlEngine* / QQmlContext*（未加载返回 nullptr）
    void* Engine() const;
    void* RootContext() const;
};

} // namespace qml
} // namespace pyarx
