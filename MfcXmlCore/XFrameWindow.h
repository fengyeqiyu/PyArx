// XFrameWindow.h —— 无框顶层外壳的宿主层能力（对标 Qt 的 QWindow 一等公民）。
//
// 为什么放在 MfcXmlCore：
//   Qt 里 FramelessWindowHint、startSystemMove()、startSystemResize()、窗口描边剥离
//   都是 QtGui/QtQuick 核心的能力，不是各例程各写一套。这里同样下沉到核心库，
//   设计器（UIDesign）与预览器（Viewer）共用同一份命中码/系统循环/描边剥离实现，
//   外壳行为才不会三端跑偏。
//
// 约定：本头只做窗口层的事（命中、系统循环、DWM 属性），不参与节点树与绘制。
// 纯 header-only，所有函数 inline，避免给各 exe 引入额外 obj 依赖。
#pragma once

#ifndef _AFXWIN_H_
#include <afxwin.h>
#endif
#include <windows.h>
#include <windowsx.h>   // GET_X_LPARAM / GET_Y_LPARAM

namespace xmlframe
{
// 边掩码：1=左 2=右 4=上 8=下（与 Qt 的 Qt::Edges 同构）
enum EdgeBits : int
{
    kEdgeNone = 0,
    kEdgeLeft = 1,
    kEdgeRight = 2,
    kEdgeTop = 4,
    kEdgeBottom = 8
};

// 贴边命中厚度（逻辑像素）。与设计器一致：6px 在 150% DPI 下约 9px，手够得到又不挡内容。
constexpr int kEdgeThickness = 6;

// 客户区坐标 pt → 边掩码。client 是本窗口客户区矩形。
inline int edgesAt(const CRect& client, const CPoint& pt, int thickness = kEdgeThickness)
{
    int edges = kEdgeNone;
    if (pt.x < thickness)                    edges |= kEdgeLeft;
    if (pt.x >= client.right - thickness)    edges |= kEdgeRight;
    if (pt.y < thickness)                    edges |= kEdgeTop;
    if (pt.y >= client.bottom - thickness)   edges |= kEdgeBottom;
    return edges;
}

// 屏幕坐标 pt → 边掩码（以顶层窗口矩形为准，供子窗口转发用）
inline int edgesAtScreen(const RECT& windowRect, const POINT& screenPt,
                         int thickness = kEdgeThickness)
{
    int edges = kEdgeNone;
    if (screenPt.x - windowRect.left < thickness)     edges |= kEdgeLeft;
    if (windowRect.right - screenPt.x < thickness)    edges |= kEdgeRight;
    if (screenPt.y - windowRect.top < thickness)      edges |= kEdgeTop;
    if (windowRect.bottom - screenPt.y < thickness)   edges |= kEdgeBottom;
    return edges;
}

// 边掩码 → 系统非客户区命中码。0 表示不是边（交给调用方当客户区处理）。
inline int hitCodeForEdges(int edges)
{
    switch (edges & (kEdgeLeft | kEdgeRight | kEdgeTop | kEdgeBottom))
    {
    case kEdgeLeft:                      return HTLEFT;
    case kEdgeRight:                     return HTRIGHT;
    case kEdgeTop:                       return HTTOP;
    case kEdgeBottom:                    return HTBOTTOM;
    case kEdgeLeft | kEdgeTop:           return HTTOPLEFT;
    case kEdgeRight | kEdgeTop:          return HTTOPRIGHT;
    case kEdgeLeft | kEdgeBottom:        return HTBOTTOMLEFT;
    case kEdgeRight | kEdgeBottom:       return HTBOTTOMRIGHT;
    default:                             return 0;
    }
}

// 边掩码 → 系统光标。0 表示用默认箭头。
inline LPCWSTR cursorForEdges(int edges)
{
    switch (edges & (kEdgeLeft | kEdgeRight | kEdgeTop | kEdgeBottom))
    {
    case kEdgeLeft:
    case kEdgeRight:                     return IDC_SIZEWE;
    case kEdgeTop:
    case kEdgeBottom:                    return IDC_SIZENS;
    case kEdgeLeft | kEdgeTop:
    case kEdgeRight | kEdgeBottom:       return IDC_SIZENWSE;
    case kEdgeRight | kEdgeTop:
    case kEdgeLeft | kEdgeBottom:        return IDC_SIZENESW;
    default:                             return nullptr;
    }
}

// Qt QWindow::startSystemMove 的等价实现：ReleaseCapture 后给顶层窗口发
// WM_NCLBUTTONDOWN(HTCAPTION)。DefWindowProc 随即进入系统移动循环，DWM 合成整窗。
// 返回 false 表示参数不可用（窗口已销毁 / 已最大化）。
inline bool startSystemMove(HWND hwnd, const CPoint& screenPt)
{
    if (hwnd == nullptr || !::IsWindow(hwnd))
        return false;
    HWND root = ::GetAncestor(hwnd, GA_ROOT);
    if (root == nullptr || ::IsZoomed(root))
        return false;
    if (::GetCapture() == hwnd)
        ::ReleaseCapture();
    ::PostMessageW(root, WM_NCLBUTTONDOWN, HTCAPTION,
                   MAKELPARAM(static_cast<short>(screenPt.x), static_cast<short>(screenPt.y)));
    return true;
}

// Qt QWindow::startSystemResize 的等价实现。edges 为 xmlframe 的边掩码。
inline bool startSystemResize(HWND hwnd, const CPoint& screenPt, int edges)
{
    if (hwnd == nullptr || !::IsWindow(hwnd))
        return false;
    HWND root = ::GetAncestor(hwnd, GA_ROOT);
    if (root == nullptr || ::IsZoomed(root))
        return false;
    const int hit = hitCodeForEdges(edges);
    if (hit == 0)
        return false;
    if (::GetCapture() == hwnd)
        ::ReleaseCapture();
    ::PostMessageW(root, WM_NCLBUTTONDOWN, static_cast<WPARAM>(hit),
                   MAKELPARAM(static_cast<short>(screenPt.x), static_cast<short>(screenPt.y)));
    return true;
}

// 去掉窗口管理器画在外面的那圈东西：
//   * 圆角（DWMWA_WINDOW_CORNER_PREFERENCE = DWMWCP_DONOTROUND）
//   * 描边色（DWMWA_BORDER_COLOR = DWMWA_COLOR_NONE）
//   * 可见边框厚度（DWMWA_VISIBLE_FRAME_BORDER_THICKNESS = 0，系统不支持时返回 E_INVALIDARG）
//   * 非客户区渲染（DWMWA_NCRENDERING_POLICY = DWMNCRP_DISABLED），失焦时那一圈白框由它兜底
// 尺寸变化、激活状态切换后必须重设一次：DWM 会按新状态把描边重画回去。
// 返回值按属性顺序写出 HRESULT，便于诊断（可传 nullptr）。
inline void stripDwmFrame(HWND hwnd, HRESULT* results = nullptr)
{
    HRESULT local[4] = { E_FAIL, E_FAIL, E_FAIL, E_FAIL };
    HRESULT* hr = (results != nullptr) ? results : local;
    HMODULE dwm = ::LoadLibraryW(L"dwmapi.dll");
    if (dwm == nullptr)
        return;
    typedef HRESULT(WINAPI* SetAttribute)(HWND, DWORD, LPCVOID, DWORD);
    SetAttribute setAttribute = reinterpret_cast<SetAttribute>(
        ::GetProcAddress(dwm, "DwmSetWindowAttribute"));
    if (setAttribute != nullptr && hwnd != nullptr && ::IsWindow(hwnd))
    {
        DWORD doNotRound = 1;            // DWMWCP_DONOTROUND
        hr[0] = setAttribute(hwnd, 33, &doNotRound, sizeof(doNotRound));
        DWORD colorNone = 0xFFFFFFFE;    // DWMWA_COLOR_NONE
        hr[1] = setAttribute(hwnd, 34, &colorNone, sizeof(colorNone));
        DWORD noBorder = 0;
        hr[2] = setAttribute(hwnd, 37, &noBorder, sizeof(noBorder));
        DWORD ncDisabled = 1;            // DWMNCRP_DISABLED
        hr[3] = setAttribute(hwnd, 2, &ncDisabled, sizeof(ncDisabled));
    }
    ::FreeLibrary(dwm);
}

// 无框窗口的样式组合：可缩放但要非客户区为 0，最小/最大化由调用方自绘窗口按钮时保留。
inline DWORD framelessStyle()
{
    return WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN;
}

// 普通 Win32 窗口过程用的无框处理（不是 MFC 程序也能用同一套外壳规则）：
//   WM_NCCALCSIZE / WM_NCACTIVATE / WM_NCUAHDRAW* / WM_NCHITTEST / WM_GETMINMAXINFO
// handled 返回 true 表示这条消息已被外壳吃掉，调用方直接 return result。
inline LRESULT framelessNcProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                               bool& handled, int captionHeight = 0,
                               int captionRightInset = 0, int minTrackW = 320,
                               int minTrackH = 200)
{
    handled = true;
    switch (msg)
    {
    case WM_NCCALCSIZE:
        // 客户区 = 整个窗口矩形，不留系统边框/标题栏
        if (wp != 0 && lp != 0)
        {
            NCCALCSIZE_PARAMS* p = reinterpret_cast<NCCALCSIZE_PARAMS*>(lp);
            RECT wnd = p->rgrc[0];
            p->rgrc[0] = wnd;
        }
        return 0;

    case WM_NCACTIVATE:
        return TRUE;                 // 失焦时不画非激活边框

    case 0x00AE:                     // WM_NCUAHDRAWCAPTION
    case 0x00AF:                     // WM_NCUAHDRAWFRAME
        return 0;

    case WM_GETMINMAXINFO:
        if (lp != 0)
        {
            MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lp);
            HMONITOR mon = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = {};
            mi.cbSize = sizeof(mi);
            if (::GetMonitorInfoW(mon, &mi))
            {
                mmi->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
                mmi->ptMaxPosition.y = mi.rcWork.top - mi.rcMonitor.top;
                mmi->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left;
                mmi->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
                mmi->ptMaxTrackSize.x = mmi->ptMaxSize.x;
                mmi->ptMaxTrackSize.y = mmi->ptMaxSize.y;
                mmi->ptMinTrackSize.x = minTrackW;
                mmi->ptMinTrackSize.y = minTrackH;
            }
        }
        return 0;

    case WM_NCHITTEST:
    {
        if (::IsZoomed(hwnd))
        {
            handled = false;
            return 0;
        }
        RECT wr;
        ::GetWindowRect(hwnd, &wr);
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        const int hit = hitCodeForEdges(edgesAtScreen(wr, pt));
        if (hit != 0)
            return hit;

        if (captionHeight > 0)
        {
            RECT client;
            ::GetClientRect(hwnd, &client);
            const int localX = pt.x - wr.left;
            const int localY = pt.y - wr.top;
            if (localY >= 0 && localY < captionHeight &&
                localX < client.right - captionRightInset)
                return HTCAPTION;
        }
        handled = false;
        return 0;
    }

    default:
        handled = false;
        return 0;
    }
}

} // namespace xmlframe

namespace xmlframe
{

// LiveResize —— 系统缩放循环期间"不布局、不重绘"的统一判据。
//
// Qt 靠 resizeEvent 的节流 + 一次 layout 完成；Win32 上 WM_ENTERSIZEMOVE / WM_EXITSIZEMOVE
// 在自绘外壳和脚本驱动的 SetWindowPos 上会丢，所以判据只能从消息间隔推：
//   * 两次 WM_SIZE 间隔 < 120ms  → 正在拖（进入冻结态，不重排不重画）
//   * 最后一次 WM_SIZE 已过去 >= 200ms 且定时器到期 → 真静止，收尾一次
// 阈值的下界必须大于单步处理时间，否则每步都误判"松手"，退化成每帧全量重绘。
class LiveResize
{
public:
    // OnSize 调用：返回 true = 当前处于拖动态（调用方应冻结绘制并 SetTimer 等收尾）
    bool noteSize(ULONGLONG now)
    {
        const bool continuous = (m_last != 0 && now - m_last < 120);
        m_last = now;
        if (continuous || m_active)
            m_active = true;
        return m_active;
    }

    // OnTimer 调用：返回 true = 尺寸已静止，可以收尾（并已自动复位）
    bool settleDue(ULONGLONG now, ULONGLONG quietMs = 200)
    {
        if (!m_active)
            return false;
        if (now - m_last < quietMs)
            return false;
        m_active = false;
        return true;
    }

    // 系统循环显式通知（有就更好，没有也不影响）
    void begin() { m_active = true; }
    void end() { m_active = false; }

    bool active() const { return m_active; }
    void reset() { m_active = false; m_last = 0; }

private:
    bool      m_active = false;
    ULONGLONG m_last = 0;
};

} // namespace xmlframe
