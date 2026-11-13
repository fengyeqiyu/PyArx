// PyArxQml.cpp —— QmlHost 实现：QGuiApplication（或复用宿主实例）+ 泵 + QQuickView + SetParent。
#include "PyArxQml.h"
#include "QmlBackend.h"
#include "PyArxQmlLog.h"

#include "windows.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QLibraryInfo>
#include <QtCore/qlogging.h>
#include <QtCore/QPluginLoader>
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QWindow>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickView>
#include <QtQml/qqml.h>
#include "SvgGraphItem.h"
#include <QtQuick/QQuickItem>
#include <QtCore/QStringList>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>
#include <string>
#include <vector>

namespace pyarx {
namespace qml {

static QGuiApplication* s_app = nullptr;
static bool s_owned = false;   // true = 由本模块新建；false = 复用宿主已有实例
static QQuickView*   s_view = nullptr;
static QmlBackend*   s_backend = nullptr;
static HWND          s_parent = nullptr;
static HWND          s_pumpWnd = nullptr;
static long long     s_maxTick = 0;
static int           s_ticks = 0;
static int           s_frames = 0;
static QElapsedTimer s_timer;

static double SinceStart()
{
    return s_timer.isValid() ? (double)s_timer.nsecsElapsed() / 1e6 : 0.0;
}

static std::wstring ExeDir()
{
    wchar_t exe[MAX_PATH] = {};
    ::GetModuleFileNameW(nullptr, exe, MAX_PATH);
    std::wstring dir(exe);
    return dir.substr(0, dir.find_last_of(L"\\/"));
}

// 本 pyd 所在目录（部署时 qml\ 和缺失 DLL 都放这里）
static std::wstring SelfDir()
{
    HMODULE self = nullptr;
    ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         (LPCWSTR)(void*)&SelfDir, &self);
    wchar_t path[MAX_PATH] = {};
    if (self) ::GetModuleFileNameW(self, path, MAX_PATH);
    std::wstring s(path);
    if (s.empty()) return ExeDir();
    return s.substr(0, s.find_last_of(L"\\/"));
}

static bool DirExists(const std::wstring& d)
{
    return ::GetFileAttributesW(d.c_str()) != INVALID_FILE_ATTRIBUTES;
}

// 缺失 Qt6 DLL 的搜索顺序：环境变量 PYARXQML_DEPS_DIR > 插件目录 > SDK bin > CAD 目录
static void PreloadQmlDeps()
{
    // 只预载真正可能缺失、且依赖闭合的模块。
    // Qt6OpenGLWidgets.dll 依赖 Qt6Widgets.dll（本项目已不再依赖 Widgets），Qt6QmlLocalStorage.dll
    // 依赖 Qt6Sql，预载它们只会失败并刷日志，已移出清单。
    static const wchar_t* kDeps[] = {
        L"Qt6QmlWorkerScript.dll", L"Qt6QmlXmlListModel.dll",
        L"Qt6QmlCore.dll", L"Qt6QuickTemplates2.dll", L"Qt6QuickControls2Impl.dll",
        L"Qt6QuickControls2.dll", L"Qt6QuickLayouts.dll", L"Qt6QuickShapes.dll",
        L"Qt6QuickParticles.dll",
        nullptr
    };

    std::vector<std::wstring> dirs;
    const wchar_t* envDir = _wgetenv(L"PYARXQML_DEPS_DIR");
    if (envDir && *envDir) dirs.push_back(envDir);
    dirs.push_back(SelfDir());
    dirs.push_back(L"G:\\Qt\\6.2.6\\msvc2019_64\\bin");
    dirs.push_back(ExeDir());

    int okCount = 0, failCount = 0;
    for (int i = 0; kDeps[i]; ++i) {
        if (::GetModuleHandleW(kDeps[i])) continue;
        HMODULE h = nullptr;
        DWORD lastErr = 0;
        std::wstring lastPath;
        for (size_t d = 0; d < dirs.size() && !h; ++d) {
            const std::wstring path = dirs[d] + L"\\" + kDeps[i];
            if (!DirExists(dirs[d])) continue;
            lastPath = path;
            h = ::LoadLibraryW(path.c_str());
            if (h) { LogWrite("预载 %ls 成功 <- %ls", kDeps[i], dirs[d].c_str()); }
            else { lastErr = ::GetLastError(); }
        }
        if (h) ++okCount;
        else {
            ++failCount;
            // 区分"文件不存在"和"存在但加载失败（缺依赖/位数不符）"：后者是真实错误，必须报出来
            LogWrite("预载 %ls 失败：最后尝试 %ls，Win32 错误 %lu（2=文件不存在，126=缺依赖模块）",
                     kDeps[i], lastPath.c_str(), (unsigned long)lastErr);
        }
    }
    LogWrite("预载汇总: 成功 %d 个，失败 %d 个", okCount, failCount);
}

static void ConfigEnv()
{
    const std::wstring dir = ExeDir();

    // QPA 平台插件目录：优先自己的（部署时 platforms\ 跟 pyd 同级），最后才退到 CAD 自带的
    // 历史版本无条件用 CAD 的 Plugins\platforms，导致 CAD 进程内走 Autodesk 那份 qwindows.dll，
    // 平台插件来源与 python 独立进程不一致，是 QGuiApplication 构造 3.2s 的首要嫌疑。
    std::wstring qpaDir;
    {
        std::vector<std::wstring> cand;
        const wchar_t* envQpa = _wgetenv(L"PYARXQML_QPA_DIR");
        if (envQpa && *envQpa) cand.push_back(envQpa);
        cand.push_back(SelfDir() + L"\\platforms");                       // 与 pyd 同级（部署形态）
        cand.push_back(SelfDir() + L"\\plugins\\platforms");
        cand.push_back(dir + L"\\Plugins\\platforms");                     // CAD 自带
        // SDK 放最后：它的父目录 plugins\ 会被 Qt 一并加入库搜索路径，
        // 首次建视图时要挨个扫 imageformats 等插件，实测 setSource 从 ~120ms 涨到 ~500ms
        cand.push_back(L"G:\\Qt\\6.2.6\\msvc2019_64\\plugins\\platforms");
        for (size_t i = 0; i < cand.size(); ++i) {
            if (!DirExists(cand[i])) continue;
            if (cand[i].find(L"qwindows") != std::wstring::npos) continue;
            qpaDir = cand[i];
            break;
        }
    }
    if (!qpaDir.empty()) {
        qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
                QDir::toNativeSeparators(QString::fromWCharArray(qpaDir.c_str())).toUtf8());
        // 把库搜索路径钉死成这一个目录：Qt 默认会把 "Qt6Core.dll 所在目录的父级/plugins" 也塞进来。
        // 注：实测 CAD 的 plugins\ 只有 9 个文件 2.3MB，所以"扫一堆 DLL"这条假设已被排除，
        // 钉死路径保留是因为它能顺带避免扫描 SDK 的 plugins 根目录（那一步会让 setSource 变慢）。
        // setLibraryPaths 允许在 QCoreApplication 构造之前调用。
        QCoreApplication::setLibraryPaths(QStringList() << QDir::toNativeSeparators(QString::fromWCharArray(qpaDir.c_str())));
        LogWrite("QPA 平台插件目录 = %ls（库搜索路径已钉死为这一个目录）", qpaDir.c_str());

        // 诊断：把"加载 qwindows.dll"和"读它的插件元数据"分别计时。
        // QPA 日志显示构造过程有两段各约 1.55s 的空白，正好对应这两件事，
        // 这里分开量，才能判断是 LoadLibrary 被外部放慢，还是元数据解析慢。
        {
            const std::wstring qwin = qpaDir + L"\\qwindows.dll";
            if (::GetFileAttributesW(qwin.c_str()) != INVALID_FILE_ATTRIBUTES) {
                QElapsedTimer t;
                t.start();
                HMODULE h = ::LoadLibraryW(qwin.c_str());   // 不释放：Qt 稍后本来也要加载
                LogWrite("诊断: LoadLibraryW(qwindows.dll) 耗时 %.1f ms（%s）",
                         (double)t.elapsed(), h ? "成功" : "失败");

                t.restart();
                QPluginLoader pl(QString::fromStdWString(qwin));
                const QJsonObject md = pl.metaData();
                LogWrite("诊断: 读取 qwindows 插件元数据耗时 %.1f ms（键 %d 个）",
                         (double)t.elapsed(), (int)md.size());
            }
        }
    } else {
        LogWrite("QPA 平台插件目录 = 未找到（将依赖 Qt 默认探测）");
    }

    // 通用插件路径（imageformats / styles 等）：默认不设。
    // 实测把 QT_PLUGIN_PATH 指到 SDK 的 plugins 根，会让 Qt 在首次建视图时挨个扫描
    // imageformats 下几十个插件 DLL，setSource 从 123ms 涨到 775ms。需要时用环境变量显式打开。
    const wchar_t* optRoot = _wgetenv(L"PYARXQML_PLUGIN_PATH");
    if (optRoot && *optRoot) {
        qputenv("QT_PLUGIN_PATH",
                QDir::toNativeSeparators(QString::fromWCharArray(optRoot)).toUtf8());
        LogWrite("QT_PLUGIN_PATH = %ls（由 PYARXQML_PLUGIN_PATH 指定）", optRoot);
    }

    // QML 模块搜索路径：插件自己的 qml\ 优先，再 CAD 的 qml\（如有），最后 SDK
    std::vector<std::wstring> qmlDirs;
    const wchar_t* envQml = _wgetenv(L"PYARXQML_QML_DIR");
    if (envQml && *envQml) qmlDirs.push_back(envQml);
    qmlDirs.push_back(SelfDir() + L"\\qml");
    qmlDirs.push_back(dir + L"\\qml");
    qmlDirs.push_back(L"G:\\Qt\\6.2.6\\msvc2019_64\\qml");

    QStringList list;
    for (size_t i = 0; i < qmlDirs.size(); ++i) {
        if (!DirExists(qmlDirs[i])) continue;
        const QString s = QDir::toNativeSeparators(QString::fromWCharArray(qmlDirs[i].c_str()));
        if (!list.contains(s)) list.append(s);
    }
    qputenv("QML2_IMPORT_PATH", list.join(';').toUtf8());
    LogWrite("QML2_IMPORT_PATH = %s", list.join(';').toUtf8().constData());
}

static void PumpInner()
{
    if (!s_app) return;
    QElapsedTimer tick;
    tick.start();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    const long long d = tick.elapsed();
    if (d > s_maxTick) s_maxTick = d;
    ++s_ticks;

    if (s_parent && s_view && ::IsWindow(s_parent)) {
        RECT rc;
        ::GetClientRect(s_parent, &rc);
        const int w = (int)(rc.right - rc.left);
        const int h = (int)(rc.bottom - rc.top);
        if (w > 0 && h > 0 && (s_view->width() != w || s_view->height() != h))
            s_view->resize(w, h);
    }
}

static LRESULT CALLBACK PumpProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_TIMER && wp == 1) { PumpInner(); return 0; }
    return ::DefWindowProcW(h, msg, wp, lp);
}

static bool EnsurePump()
{
    if (s_pumpWnd) return true;
    static const wchar_t* kClass = L"PyArxQmlPump";
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = PumpProc;
    wc.hInstance = ::GetModuleHandleW(nullptr);
    wc.lpszClassName = kClass;
    if (!::RegisterClassExW(&wc)) { LogWrite("RegisterClassExW 失败 %lu", ::GetLastError()); return false; }
    s_pumpWnd = ::CreateWindowExW(0, kClass, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
    if (!s_pumpWnd) { LogWrite("CreateWindowExW 失败 %lu", ::GetLastError()); return false; }
    if (!::SetTimer(s_pumpWnd, 1, 16, nullptr)) { LogWrite("SetTimer 失败 %lu", ::GetLastError()); return false; }
    LogWrite("泵定时器已装（16 ms）");
    return true;
}

QmlHost& QmlHost::Instance()
{
    static QmlHost host;
    return host;
}

bool QmlHost::CoreOnlyProbe()
{
    if (s_app) {
        LogWrite("CoreOnlyProbe：进程内已有应用实例，跳过");
        return false;
    }
    s_timer.start();
    ConfigEnv();
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    PreloadQmlDeps();
    LogWrite("CoreOnlyProbe 环境与预载完成 +%.1f ms（只建 QCoreApplication，不建 GUI）", SinceStart());

    static int argc = 1;
    static char arg0[] = "acad";
    static char* argv[] = { arg0, nullptr };
    const double t0 = SinceStart();
    QCoreApplication* core = new QCoreApplication(argc, argv);
    LogWrite("QCoreApplication 构造完成 +%.1f ms（构造本身耗时 %.1f ms）", SinceStart(), SinceStart() - t0);
    {
        const QStringList paths = QCoreApplication::libraryPaths();
        LogWrite("库搜索路径共 %d 条：%s", (int)paths.size(),
                 paths.join(QStringLiteral(" | ")).toUtf8().constData());
    }
    (void)core;
    return true;
}

bool QmlHost::Ensure(bool preloadDeps)
{
    if (s_app) return EnsurePump();

    s_timer.start();   // 计时覆盖：环境配置 -> DLL 预载 -> 应用实例构造

    // 版本闸：确认进程内实际加载的 Qt 与编译时一致。
    // CAD 升级后若撞上同名但不同版本的 Qt DLL，宁可拒绝启动也不要进去崩。
    {
        const char* loaded = qVersion();
        const wchar_t* allowEnv = _wgetenv(L"PYARXQML_ALLOW_QT_MISMATCH");
        const bool allow = (allowEnv && *allowEnv && *allowEnv != L'0');
        if (loaded && ::strcmp(loaded, QT_VERSION_STR) != 0 && !allow) {
            LogWrite("Qt 版本闸拒绝：进程内 Qt=%s，本模块编译时=%s（设 PYARXQML_ALLOW_QT_MISMATCH=1 可跳过）",
                     loaded, QT_VERSION_STR);
            return false;
        }
        LogWrite("Qt 版本闸通过：%s", loaded ? loaded : "未知");
    }
    ConfigEnv();
    // 诊断开关：PYARXQML_QPA_LOG=1 时把 Qt 自己的 qpa 日志也写进同一份日志（带毫秒时间戳），
    // 用来定位 QGuiApplication 构造到底卡在平台层的哪一步。
    {
        const wchar_t* qpaLog = _wgetenv(L"PYARXQML_QPA_LOG");
        if (qpaLog && *qpaLog && *qpaLog != L'0') {
            qputenv("QT_LOGGING_RULES", "qt.qpa.*=true");
            qInstallMessageHandler([](QtMsgType, const QMessageLogContext&, const QString& msg) {
                LogWrite("[qt] %s", msg.toUtf8().constData());
            });
            LogWrite("已开启 Qt QPA 日志（PYARXQML_QPA_LOG=1）");
        }
    }
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    if (preloadDeps) PreloadQmlDeps();
    LogWrite("环境与预载完成 +%.1f ms", SinceStart());

    // 宿主（CAD 等）若已经建过 QGuiApplication，直接复用：省掉整套平台插件、字体与样式初始化
    QCoreApplication* existing = QCoreApplication::instance();
    if (existing) {
        if (QGuiApplication* hostApp = qobject_cast<QGuiApplication*>(existing)) {
            s_app = hostApp;
            s_owned = false;
            LogWrite("复用宿主已有 QGuiApplication，跳过重复初始化 +%.1f ms", SinceStart());
            return EnsurePump();
        }
        LogWrite("宿主已有 QCoreApplication 但非 QGuiApplication，无法复用，将新建实例");
    } else {
        LogWrite("宿主无 QCoreApplication 实例（QCoreApplication::instance() == nullptr），必须新建");
    }

    static int argc = 1;
    static char arg0[] = "acad";
    static char* argv[] = { arg0, nullptr };
    const double t0 = SinceStart();
    s_app = new QGuiApplication(argc, argv);
    s_owned = true;
    s_app->setQuitOnLastWindowClosed(false);
    LogWrite("QGuiApplication 构造完成 +%.1f ms（构造本身耗时 %.1f ms）", SinceStart(), SinceStart() - t0);
    LogWrite("平台=%s 插件路径=%s",
             QGuiApplication::platformName().toUtf8().constData(),
             QLibraryInfo::path(QLibraryInfo::PluginsPath).toUtf8().constData());
    {
        const QStringList paths = QCoreApplication::libraryPaths();
        LogWrite("库搜索路径共 %d 条：%s", (int)paths.size(),
                 paths.join(QStringLiteral(" | ")).toUtf8().constData());
    }

    return EnsurePump();
}

bool QmlHost::Attach(const std::wstring& qmlFile, void* parentHwnd, int width, int height)
{
    LogWrite("===== Attach(%ls, parent=%p, %dx%d) =====", qmlFile.c_str(), parentHwnd, width, height);
    if (!Ensure(true)) return false;

    s_parent = (HWND)parentHwnd;

    // 注册配电干线图用的矢量 SVG 图元（QSvgRenderer + QQuickPaintedItem）
    static bool s_typesRegistered = false;
    if (!s_typesRegistered) {
        qmlRegisterType<SvgGraphItem>("PyArx.PowerPlan", 1, 0, "SvgGraph");
        s_typesRegistered = true;
        LogWrite("已注册 QML 类型 PyArx.PowerPlan 1.0 SvgGraph");
    }

    s_view = new QQuickView();
    s_view->setResizeMode(QQuickView::SizeRootObjectToView);
    s_view->resize(width, height);

    if (!s_backend) s_backend = new QmlBackend();
    s_view->rootContext()->setContextProperty("backend", s_backend);

    QObject::connect(s_view, &QQuickWindow::frameSwapped, [](void) {
        ++s_frames;
        if (s_frames <= 3 || s_frames == 10)
            LogWrite("第 %d 帧已交换 +%.1f ms", s_frames, SinceStart());
    });

    s_view->setSource(QUrl::fromLocalFile(QString::fromWCharArray(qmlFile.c_str())));
    LogWrite("setSource 完成 status=%d +%.1f ms", (int)s_view->status(), SinceStart());
    if (s_view->status() == QQuickView::Error) {
        for (const auto& e : s_view->errors())
            LogWrite("QML 错误: %s", e.toString().toUtf8().constData());
        return false;
    }

    s_view->show();

    if (s_parent && ::IsWindow(s_parent)) {
        RECT rc = {};
        ::GetClientRect(s_parent, &rc);
        HWND qtHwnd = (HWND)s_view->winId();
        LogWrite("宿主客户区 %dx%d，QQuickView 窗口 %p", (int)(rc.right - rc.left), (int)(rc.bottom - rc.top), qtHwnd);
        ::SetParent(qtHwnd, s_parent);
        LONG_PTR style = ::GetWindowLongPtrW(qtHwnd, GWL_STYLE);
        style = (style & ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME)) | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        ::SetWindowLongPtrW(qtHwnd, GWL_STYLE, style);
        ::SetWindowPos(qtHwnd, HWND_TOP, 0, 0, width, height, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        LogWrite("SetParent 完成 +%.1f ms", SinceStart());
    }
    return true;
}

void QmlHost::Pump() { PumpInner(); }

void QmlHost::Stop()
{
    if (s_pumpWnd) { ::KillTimer(s_pumpWnd, 1); LogWrite("泵定时器已停"); }
}

void* QmlHost::Hwnd() const
{
    if (!s_view) return nullptr;
    return (void*)(unsigned long long)s_view->winId();
}

int QmlHost::RenderBackend() const
{
    if (!s_view) return 0;
    QSGRendererInterface* ri = s_view->rendererInterface();
    if (!ri) return 0;
    return (int)ri->graphicsApi();
}

HostStats QmlHost::Stats() const
{
    HostStats st;
    st.ticks = s_ticks;
    st.maxTickMs = s_maxTick;
    st.frames = s_frames;
    st.hasApp = (s_app != nullptr);
    st.hasView = (s_view != nullptr);
    return st;
}

bool QmlHost::Grab(const std::wstring& pngPath) const
{
    if (!s_view) return false;
    PumpInner();
    const bool ok = s_view->grabWindow().save(QString::fromWCharArray(pngPath.c_str()));
    LogWrite("抓图 %s -> %ls", ok ? "成功" : "失败", pngPath.c_str());
    return ok;
}

bool QmlHost::PushStatus(const std::string& text)
{
    if (!s_backend) return false;
    s_backend->setStatus(QString::fromUtf8(text.c_str()));
    LogWrite("Python 推送到 QML: %s", text.c_str());
    return true;
}

std::string QmlHost::QueryProp(const std::string& name)
{
    if (!s_backend) return std::string();
    const QVariant v = s_backend->property(name.c_str());
    if (!v.isValid()) return std::string();
    return std::string(v.toString().toUtf8().constData());
}

void* QmlHost::RootObject() const
{
    if (!s_view) return nullptr;
    return (void*)s_view->rootObject();
}

void* QmlHost::FindItem(const std::string& objectName) const
{
    if (!s_view) return nullptr;
    QObject* root = s_view->rootObject();
    if (!root) return nullptr;
    if (objectName.empty()) return (void*)root;
    const QString name = QString::fromUtf8(objectName.c_str());
    QObject* o = root->findChild<QObject*>(name);
    return (void*)o;
}

void* QmlHost::Engine() const
{
    if (!s_view) return nullptr;
    return (void*)s_view->engine();
}

void* QmlHost::RootContext() const
{
    if (!s_view) return nullptr;
    return (void*)s_view->rootContext();
}

} // namespace qml
} // namespace pyarx
