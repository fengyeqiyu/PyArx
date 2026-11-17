// PyMfcCommon.h —— 公共头/宏/辅助函数/前向声明（PyMfcXml 各编译单元共享）
#pragma once

// PyMfcCore.cpp — MFC 原生控件的最小 Python 接口。
//
// 生命周期原则（2026-11-04 重构）：
//   1) 所有 MFC 原生对象（窗口、控件容器）由进程级单例
//      PyMfcStore 持有，生命周期到 CAD 进程结束；
//   2) Python 侧对象只持有裸指针，析构时**不 delete** 原生对象，
//      避免卸载栈内销毁窗口导致的堆损坏（0xC0000374）；
//   3) shutdown() 只做隐藏 + 异步关闭窗口，不在 arxunload 调用栈里销毁窗口；
//   4) 侧栏（CAdUiPaletteSet）不在本库：由 PyArxCore.Ap 提供，本库只认页 HWND。
//
// Python 只操作：Window / Page / PaletteSet 上的控件 API（完整清单见交接文档 12.7 节）：
//   add_button / add_label / add_edit / add_listbox / add_checkbox / add_radio /
//   add_group / add_combo / add_progress / add_slider
//   set_text / get_text / set_visible / get_visible / set_enabled / get_enabled /
//   move / set_focus / set_readonly
//   add_string / clear_strings / count_strings / get_string / delete_string /
//   get_cur_sel / set_cur_sel
//   get_check / set_check / set_range / set_pos / get_pos
//   on_click / on_change / on_select
// 所有 HWND 与消息映射归 C++，Python 不接触 CWnd*。

#include "stdafx.h"
#include "XmlCore.h"
#include "XmlDraw.h"     // 三端共用的"声明 → 绘制命令"层
#include "XmlD2D.h"      // Direct2D 后端（DCRenderTarget，画到 owner-draw 的 HDC 上）
#include "XmlControls.h" // 共享绘制器（树/页签/列表等）
#include "PyMfcSkin.h"    // 原生控件的自绘皮肤（绘制权收回 D2D 层）
#include "controls/XmlMfcHost.h"
#include "controls/XmlMfcControlBase.h"
#include "controls/RegisterPyMfcControls.h"
#include "XmlBindings.h"   // PyMfcCore.xml 子模块：把 PyMfcXml 加载器暴露给 Python
#include "XmlPowerPlan.h"  // 配电规划图：建树/布局/XML/JSON/CSV + 离屏渲染
#include "XmlPowerPlanDraw.h"  // 唯一一份配电图画法（屏幕自绘 / 预览 / 设计器 / 离屏共用）
#include "XmlGeometry.h"   // sCurvePoints：回路 S 曲线采样
#include "XmlSvgScene.h"   // SVG：nanosvg 解析 → XmlCanvas 绘制
#include <algorithm>
#pragma comment(lib, "MfcXmlCore.lib")
#include <dwmapi.h>
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
// 图片装载（StaticBitmap / BitmapButton）：用 WIC 解码 png/jpg 再转成 HBITMAP
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
// 验收取证：savePng 用 WIC 编码，文件流走 SHCreateStreamOnFileW
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// 配电图绘制：屏幕自绘画布与离屏出图共用同一套画法。
// 参数 k = 缩放（离屏传 scale），(ox, oy) = 视口偏移（离屏传 0,0）。
// 与 HTML 版 power-plan.html 的 buildGraphString 逐项对齐：纸面 / 回路 / 电缆标注 /
// 卡片（root 深边加粗、terminal 浅蓝底）/ 电压角标 / 三行文字 / 折叠 +N。
// -----------------------------------------------------------------------------
// 画法本体已下沉到控件库（MfcXmlCore\XmlPowerPlanDraw.h）：
// 屏幕自绘画布 / Viewer 预览 / 设计器表面 / 离屏出图共用同一份实现，三端不会再各画一套。
static void drawPowerPlanContent(XmlCanvas& cv, const xmlplan::PowerPlan& plan,
                                 float k, int ox, int oy, const std::wstring& sel,
                                 bool showCable = true, const RECT* clip = nullptr)
{
    xmlplan::drawPlan(cv, plan, k, ox, oy, sel, showCable, clip);
}

namespace
{
    // ---- 本地 Python 直跑时的 MFC 自举 --------------------------------------
    // MFC 的 CWnd::Create / 子类化都依赖模块态里的 CWinApp。CAD 之类宿主本来就有，
    // 直接用；裸 python.exe 里没有，必须补一个最小 CWinApp 再 AfxWinInit，
    // 否则建窗直接访问空指针崩溃（0xC0000005）。
    class PyMfcStandaloneApp : public CWinApp
    {
    public:
        BOOL InitInstance() override { return TRUE; }
    };

    void ensureMfcApp()
    {
        if (AfxGetApp() != nullptr)
            return;                       // 宿主已提供 CWinApp（CAD 进程内）

        static PyMfcStandaloneApp* app = new PyMfcStandaloneApp();
        UNREFERENCED_PARAMETER(app);
        AfxWinInit(::GetModuleHandleW(nullptr), nullptr, ::GetCommandLineW(), 0);

        // 本地直跑没有宿主进程，进程退出阶段显卡驱动销毁设备会在 atidxx64.dll 里空指针崩溃；
        // 强制 D2D 走 WARP 软件渲染，完全不碰显卡驱动（环境变量未显式设置时才设）。
        if (::GetEnvironmentVariableW(L"PYARX_D2D_RT", nullptr, 0) == 0)
            ::SetEnvironmentVariableW(L"PYARX_D2D_RT", L"software");

        // 富文本控件类来自 Msftedit.dll；CAD 里由 acad 预先加载，本地直跑需自己载入
        ::LoadLibraryW(L"Msftedit.dll");

        // 通用控件类（ListView/Tab/DateTime 等）由宿主注册；本地直跑时自己补
        INITCOMMONCONTROLSEX icc = {};
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES |
                    ICC_TREEVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_TAB_CLASSES |
                    ICC_BAR_CLASSES | ICC_UPDOWN_CLASS | ICC_DATE_CLASSES |
                    ICC_USEREX_CLASSES;
        ::InitCommonControlsEx(&icc);
    }

    // 日志目录：PYARX_LOG_DIR 优先，否则 PyMfcCore.pyd 自身所在目录下的 log 子目录
    std::wstring pymfcLogFile(const wchar_t* fileName)
    {
        wchar_t env[1024] = {};
        const DWORD n = ::GetEnvironmentVariableW(L"PYARX_LOG_DIR", env, 1024);
        if (n >= 2)
            return std::wstring(env, n) + L"\\" + fileName;

        wchar_t path[MAX_PATH] = {};
        std::wstring dir;
        if (::GetModuleFileNameW(::GetModuleHandleW(L"PyMfcXml.pyd"), path, MAX_PATH) != 0)
        {
            dir = path;
            const size_t cut = dir.find_last_of(L"\\/");
            if (cut != std::wstring::npos)
                dir.resize(cut);
        }
        dir += L"\\log";
        ::CreateDirectoryW(dir.c_str(), nullptr);
        return dir + L"\\" + fileName;
    }

    // PyMfcCore.pyd 所在目录（Icon="res/xxx.png" 这类相对路径的基准）
    std::wstring pymfcDir()
    {
        wchar_t path[MAX_PATH] = {};
        std::wstring dir;
        if (::GetModuleFileNameW(::GetModuleHandleW(L"PyMfcXml.pyd"), path, MAX_PATH) != 0)
        {
            dir = path;
            const size_t cut = dir.find_last_of(L"\\/");
            if (cut != std::wstring::npos)
                dir.resize(cut);
        }
        return dir;
    }

    // 仅当环境变量 PYARX_MFC_DEBUG=1 时输出，便于自动化测试定位窗口状态
    void debugLogf(const wchar_t* fmt, ...)
    {
        wchar_t probe[4] = {};
        if (::GetEnvironmentVariableW(L"PYARX_MFC_DEBUG", probe, 4) == 0)
            return;
        FILE* f = _wfopen(pymfcLogFile(L"pymfc_shutdown.log").c_str(), L"a, ccs=UTF-8");
        if (f == nullptr)
            return;
        va_list args;
        va_start(args, fmt);
        vfwprintf(f, fmt, args);
        va_end(args);
        fputws(L"\n", f);
        fclose(f);
    }

    // 绘制耗时埋点：PYARX_MFC_TIMING=1 时把每次自绘的毫秒数单独记一行，
    // 用来判断"卡顿"是画得慢（一次几十毫秒）还是画得勤（同一个控件被反复重画）。
    bool paintTimingEnabled()
    {
        static int cached = -1;
        if (cached < 0)
        {
            wchar_t v[4] = {};
            cached = (::GetEnvironmentVariableW(L"PYARX_MFC_TIMING", v, 4) >= 1 && v[0] == L'1') ? 1 : 0;
        }
        return cached == 1;
    }

    void paintLogf(const wchar_t* fmt, ...)
    {
        if (!paintTimingEnabled())
            return;
        FILE* f = _wfopen(pymfcLogFile(L"pymfc_paint.log").c_str(), L"a, ccs=UTF-8");
        if (f == nullptr)
            return;
        va_list args;
        va_start(args, fmt);
        vfwprintf(f, fmt, args);
        va_end(args);
        fputws(L"\n", f);
        fclose(f);
    }
}

namespace
{
    constexpr UINT kFirstId = 1000;
    constexpr UINT kLastId = 65000;

    // 同一套“加条目”语义在 ListBox 与 ComboBox 上消息号不同，按控件类名分派
    bool isClassOf(HWND hwnd, const wchar_t* className)
    {
        if (hwnd == nullptr || className == nullptr)
            return false;
        WCHAR cls[64] = {};
        if (::GetClassNameW(hwnd, cls, 64) == 0)
            return false;
        return lstrcmpiW(cls, className) == 0;
    }

    bool isComboBox(HWND hwnd) { return isClassOf(hwnd, L"ComboBox"); }
    bool isProgressBar(HWND hwnd) { return isClassOf(hwnd, L"msctls_progress32"); }
    bool isTrackBar(HWND hwnd) { return isClassOf(hwnd, L"msctls_trackbar32"); }

    void invokePython(py::object& callback)
    {
        if (!callback)
            return;
        py::gil_scoped_acquire gil;
        try
        {
            callback();
        }
        catch (py::error_already_set& e)
        {
            // 回调出错时不能静默：stdout 在 CAD 里被重定向，错误必须落到诊断日志
            debugLogf(L"[callback] Python 回调抛异常：%hs", e.what());
            e.restore();
            PyErr_Print();
        }
        catch (...)
        {
            debugLogf(L"[callback] Python 回调抛未知异常");
            PyErr_Print();
        }
    }
}

namespace
{
    ATOM ensureNativeClass()    {
        static ATOM atom = 0;
        if (atom == 0)
        {
            WNDCLASSEXW wc = {};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = ::DefWindowProcW;
            wc.hInstance = AfxGetInstanceHandle();
            wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            wc.lpszClassName = L"PyMfcNativeWindow";
            atom = ::RegisterClassExW(&wc);
        }
        return atom;
    }
}

// 配电画布鼠标状态机：定义在 PyMfcHost 之后（要用到宿主里的画布状态），
// 这里先给前向声明，供 owner-draw 窗口过程调用。
class PyMfcHost;
bool wicWritePng(const std::wstring& path, int w, int h, void* bgra);
static constexpr const wchar_t* kGridHostProp = L"PyMfcGridHost";
static constexpr const wchar_t* kGridOldProp = L"PyMfcGridOldProc";

// 可编辑表格钩子（定义见 PyMfcPlanCanvasMouse 之前）
LRESULT CALLBACK PyMfcGridProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT PyMfcPlanCanvasMouse(PyMfcHost* host, HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam, bool& handled);

// 三维视图画布的鼠标状态机（左键旋转 / 滚轮缩放 / 中键平移 / 双击复位）
LRESULT PyMfcView3DMouse(PyMfcHost* host, HWND hwnd, UINT msg,
                         WPARAM wParam, LPARAM lParam, bool& handled);


// 滚轮拦截：把顶层窗口的窗口过程换成 PyMfcWheelProc，并登记宿主指针与原过程。
// 定义在 PyMfcProcs.cpp；返回原窗口过程，供未命中注册区域的滚轮原样转回。
WNDPROC installWheelHost(HWND hwnd, void* host);

// 手型（可点控件）/ 爪型（画布，系统光标表里没有 grab 手型，从 res\grab.cur 载入，
// 载不到就退回系统手型）。定义在 PyMfcProcs.cpp。
HCURSOR pyMfcHandCursor();
HCURSOR pyMfcGrabCursor();
// 按住拖动（panning）时的"握住的"光标：res\grab_down.cur，缺失时退回 grab
HCURSOR pyMfcGrabDownCursor();
// 画布按当前状态取光标：拖动中=握住的，静止=grab
HCURSOR pyMfcCanvasCursor(HWND hwnd);
