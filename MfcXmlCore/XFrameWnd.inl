// XFrameWnd.inl —— XFrameWnd 的实现（MFC 消息映射只能在一个编译单元里展开）。
// 每个程序指定唯一一个"外壳装配 TU"包含本文件，其余 TU 只用 XFrameWnd.h。
#include "XFrameWnd.h"
#include <stdio.h>

namespace xmlframe
{

BEGIN_MESSAGE_MAP(XFrameWnd, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_WM_ERASEBKGND()
    ON_WM_NCCALCSIZE()
    ON_WM_GETMINMAXINFO()
    ON_WM_NCHITTEST()
    ON_WM_ENTERSIZEMOVE()
    ON_WM_EXITSIZEMOVE()
    ON_WM_CLOSE()
    ON_WM_DESTROY()
    ON_WM_ACTIVATE()
    ON_WM_NCACTIVATE()
    ON_MESSAGE(0x00AE, &XFrameWnd::OnNcUahDraw)   // WM_NCUAHDRAWCAPTION
    ON_MESSAGE(0x00AF, &XFrameWnd::OnNcUahDraw)   // WM_NCUAHDRAWFRAME
END_MESSAGE_MAP()

XFrameWnd::XFrameWnd() {}
XFrameWnd::~XFrameWnd() {}

BOOL XFrameWnd::CreateFrame(LPCTSTR title, const CRect& rc, CWnd* parent, FrameMode mode)
{
    m_mode = mode;
    m_frameless = (mode == FrameFrameless);

    const DWORD style = m_frameless ? framelessStyle() : (WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN);

    // 深色背景刷：窗口首次显出来、或还没画到的像素都是底色，不会闪白
    LPCTSTR cls = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW),
                                     ::CreateSolidBrush(m_back), nullptr);
    if (!CreateEx(0, cls, title, style,
                  rc.left, rc.top, rc.Width(), rc.Height(),
                  (parent != nullptr) ? parent->GetSafeHwnd() : nullptr,
                  nullptr, nullptr))
        return FALSE;

    frameLog(L"[frame] mode=%s hwnd=%p", m_frameless ? L"frameless" : L"native", (void*)m_hWnd);
    return TRUE;
}

void XFrameWnd::SetContent(CWnd* wnd)
{
    m_content = wnd;
    layoutContent();
    if (m_content != nullptr && m_content->GetSafeHwnd() != nullptr)
        m_content->ShowWindow(SW_SHOW);
}

HWND XFrameWnd::ContentHwnd() const
{
    return (m_content != nullptr) ? m_content->GetSafeHwnd() : nullptr;
}

FrameMode XFrameWnd::Mode() const { return m_mode; }

void XFrameWnd::SetCaptionDrag(int height, int rightInset)
{
    m_captionHeight = height;
    m_captionRightInset = rightInset;
}

void XFrameWnd::SetMinTrackSize(int w, int h)
{
    m_minTrackW = w;
    m_minTrackH = h;
}

void XFrameWnd::SetFrameBackground(COLORREF color) { m_back = color; }

void XFrameWnd::layoutContent()
{
    if (m_content == nullptr || m_content->GetSafeHwnd() == nullptr)
        return;
    CRect rc;
    GetClientRect(&rc);
    if (rc.Width() > 0 && rc.Height() > 0)
        m_content->SetWindowPos(nullptr, 0, 0, rc.Width(), rc.Height(),
                                SWP_NOZORDER | SWP_NOACTIVATE);
}

void XFrameWnd::stripFrame()
{
    HRESULT hr[4] = { S_OK, S_OK, S_OK, S_OK };
    stripDwmFrame(m_hWnd, hr);
    frameLog(L"[dwm] corner=%08lX border=%08lX thickness=%08lX ncrender=%08lX",
             (unsigned long)hr[0], (unsigned long)hr[1],
             (unsigned long)hr[2], (unsigned long)hr[3]);
}

int XFrameWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;
    if (!CreateContent())
        return -1;
    layoutContent();
    if (m_frameless)
        stripFrame();
    return 0;
}

void XFrameWnd::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    if (nType == SIZE_MINIMIZED)
        return;

    // 只铺满内容窗口（每帧跟上尺寸）；DWM 属性留到尺寸稳定后再压：
    // 每次 DwmSetWindowAttribute 都会让 DWM 重算整窗合成，拖边时每帧调用等于每帧刷新整窗。
    layoutContent();
    if (m_frameless)
        SetTimer(kTimerSettle, 60, nullptr);
    OnFrameSizeChanged(cx, cy);
}

void XFrameWnd::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == kTimerSettle)
    {
        KillTimer(kTimerSettle);
        stripFrame();
        return;
    }
    CWnd::OnTimer(nIDEvent);
}

BOOL XFrameWnd::OnEraseBkgnd(CDC* pDC)
{
    CRect rc;
    GetClientRect(&rc);
    pDC->FillSolidRect(&rc, m_back);
    return TRUE;
}

// 无边框：客户区铺满整窗，不留任何系统标题栏/边框厚度
void XFrameWnd::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
    if (!m_frameless)
    {
        CWnd::OnNcCalcSize(bCalcValidRects, lpncsp);
        return;
    }
    (void)bCalcValidRects; (void)lpncsp;
    // 不调基类：基类会按 caption 高度重算，又把非客户区留回来
}

void XFrameWnd::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    if (lpMMI != nullptr)
    {
        HMONITOR mon = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        if (::GetMonitorInfoW(mon, &mi))
        {
            lpMMI->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
            lpMMI->ptMaxPosition.y = mi.rcWork.top - mi.rcMonitor.top;
            lpMMI->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left;
            lpMMI->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
            lpMMI->ptMaxTrackSize.x = lpMMI->ptMaxSize.x;
            lpMMI->ptMaxTrackSize.y = lpMMI->ptMaxSize.y;
            lpMMI->ptMinTrackSize.x = m_minTrackW;
            lpMMI->ptMinTrackSize.y = m_minTrackH;
        }
    }
    CWnd::OnGetMinMaxInfo(lpMMI);
}

LRESULT XFrameWnd::OnNcHitTest(CPoint point)
{
    if (!m_frameless || ::IsZoomed(m_hWnd))
        return CWnd::OnNcHitTest(point);

    CRect wr;
    GetWindowRect(&wr);

    // 四边 6px：交回系统缩放循环（Qt FramelessWindowHint + 自绘边的等价物）
    const int edges = edgesAtScreen({ wr.left, wr.top, wr.right, wr.bottom },
                                    { point.x, point.y });
    const int hit = hitCodeForEdges(edges);
    if (hit != 0)
        return hit;

    // 标题栏：交回系统移动循环；右侧留给窗口按钮的宽度不参与拖动
    if (m_captionHeight > 0)
    {
        CRect client;
        GetClientRect(&client);
        const int localX = point.x - wr.left;
        const int localY = point.y - wr.top;
        if (localY >= 0 && localY < m_captionHeight &&
            localX < client.Width() - m_captionRightInset)
            return HTCAPTION;
    }
    return HTCLIENT;
}

void XFrameWnd::OnEnterSizeMove() { OnFrameSizeMoveBegin(); }
void XFrameWnd::OnExitSizeMove() { OnFrameSizeMoveEnd(); }

void XFrameWnd::OnClose()
{
    if (!OnFrameClose())
        return;
    DestroyWindow();
}

void XFrameWnd::OnDestroy()
{
    CWnd::OnDestroy();
    ::PostQuitMessage(0);
}

// 激活/失焦切换时 DWM 会按新状态重画描边，切换后重压一遍让它无可画之处
void XFrameWnd::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    CWnd::OnActivate(nState, pWndOther, bMinimized);
    layoutContent();
    if (m_frameless)
        stripFrame();
}

// 失焦那圈白框来自 DefWindowProc 在 WM_NCACTIVATE 里重画非激活边框，吃掉它
BOOL XFrameWnd::OnNcActivate(BOOL bActive)
{
    if (!m_frameless)
        return CWnd::OnNcActivate(bActive);
    (void)bActive;
    return TRUE;
}

// Win10/11 的 UAH 绘制会单独补画标题/边框，一并吃掉
LRESULT XFrameWnd::OnNcUahDraw(WPARAM wParam, LPARAM lParam)
{
    (void)wParam; (void)lParam;
    return 0;
}

// 外壳诊断：PYARX_DESIGN_DEBUG=1 时写 <exe目录>\designer_host.log
void XFrameWnd::frameLog(const wchar_t* fmt, ...)
{
    wchar_t flag[8] = {};
    if (::GetEnvironmentVariableW(L"PYARX_DESIGN_DEBUG", flag, 8) != 1 || flag[0] != L'1')
        return;

    wchar_t path[MAX_PATH] = {};
    ::GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\')) *(slash + 1) = 0;
    wcscat_s(path, L"designer_host.log");

    FILE* f = nullptr;
    _wfopen_s(&f, path, L"a, ccs=UTF-8");
    if (f == nullptr) return;
    va_list ap;
    va_start(ap, fmt);
    vfwprintf(f, fmt, ap);
    va_end(ap);
    fwprintf(f, L"\n");
    fclose(f);
}

} // namespace xmlframe
