#include "stdafx.h"
#include "DesignerFrame.h"
#include <stdio.h>

using namespace uidesign;

// 宿主初始化阶段的落盘日志：停靠框架出错时进程直接退出，OutputDebugString 来不及看。
// 默认不写盘（否则每次启动都往输出目录丢一个 log 文件），排障时置 PYARX_DESIGN_DEBUG=1 再开。
static void hostLog(const wchar_t* fmt, ...)
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

BEGIN_MESSAGE_MAP(DesignerFrame, CFrameWndEx)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_ACTIVATE()
    ON_WM_NCACTIVATE()
    ON_MESSAGE(0x00AE, &DesignerFrame::OnNcUahDraw)   // WM_NCUAHDRAWCAPTION
    ON_MESSAGE(0x00AF, &DesignerFrame::OnNcUahDraw)   // WM_NCUAHDRAWFRAME
    ON_WM_NCCALCSIZE()
    ON_WM_GETMINMAXINFO()
    ON_WM_TIMER()
    ON_WM_ENTERSIZEMOVE()
    ON_WM_EXITSIZEMOVE()
    ON_WM_NCHITTEST()
    ON_WM_CLOSE()
END_MESSAGE_MAP()

BOOL DesignerFrame::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg != nullptr && pMsg->message == WM_KEYDOWN)
    {
        static const bool on = []() {
            char buf[8] = { 0 };
            return ::GetEnvironmentVariableA("PYARX_DESIGN_DEBUG", buf, sizeof(buf)) > 0;
        }();
        if (on)
        {
            std::FILE* f = nullptr;
            if (fopen_s(&f, "G:\\PyArx\\log\\paste_diag.log", "a") == 0 && f != nullptr)
            {
                fprintf(f, "[frame-key] hwnd=%p vk=%02X ctrl=%d focus=%p\n",
                        (void*)pMsg->hwnd, (unsigned)pMsg->wParam,
                        (::GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0, (void*)::GetFocus());
                fclose(f);
            }
        }
    }
    // 停靠体系在每次布局后都会把 4px 滑块重新显示出来：排到消息队尾再收一次
    if (pMsg != nullptr && pMsg->message == (WM_APP + 100))
    {
        hideDockSliders();
        return TRUE;
    }
    // 焦点在框架本身（没落到设计器视图的子树）时，Delete 也要能删掉画布/对象树里选中的节点。
    // 焦点在视图子树时 DesignerPaneView::PreTranslateMessage 会先处理并吃掉消息，这里不会重复删。
    if (pMsg != nullptr && pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_DELETE)
    {
        DesignerPaneView& view = m_pane.view();
        if (view.GetSafeHwnd() != nullptr && view.handleDeleteKey(pMsg->hwnd))
            return TRUE;
    }
    // Ctrl+Z / Ctrl+Y：焦点在框架本身时也要能撤销 / 重做。
    // 焦点在视图子树（画布/对象树/代码区）时由 DesignerPaneView::PreTranslateMessage 处理，这里跳过，避免撤两步。
    if (pMsg != nullptr && pMsg->message == WM_KEYDOWN &&
        (pMsg->wParam == 'Z' || pMsg->wParam == 'Y' || pMsg->wParam == 'z' || pMsg->wParam == 'y') &&
        (::GetKeyState(VK_CONTROL) & 0x8000) != 0)
    {
        DesignerPaneView& view = m_pane.view();
        HWND vh = view.GetSafeHwnd();
        if (vh != nullptr && pMsg->hwnd != vh && !::IsChild(vh, pMsg->hwnd))
        {
            view.undoOrRedo(pMsg->wParam == 'Z' || pMsg->wParam == 'z');
            return TRUE;
        }
    }
    // Ctrl+A 全选 / Ctrl+C/X/V 剪贴板：同上，焦点在框架本身（不在设计器视图子树）时直接按节点处理。
    // 落在视图子树里的（画布、对象树、代码区、属性内联框）交给 DesignerPaneView 自己判定，避免抢走文本编辑。
    if (pMsg != nullptr && pMsg->message == WM_KEYDOWN &&
        (pMsg->wParam == 'A' || pMsg->wParam == 'a' ||
         pMsg->wParam == 'C' || pMsg->wParam == 'X' || pMsg->wParam == 'V' ||
         pMsg->wParam == 'c' || pMsg->wParam == 'x' || pMsg->wParam == 'v') &&
        (::GetKeyState(VK_CONTROL) & 0x8000) != 0)
    {
        DesignerPaneView& view = m_pane.view();
        HWND vh = view.GetSafeHwnd();
        if (vh != nullptr && pMsg->hwnd != vh && !::IsChild(vh, pMsg->hwnd))
        {
            if (pMsg->wParam == 'A' || pMsg->wParam == 'a') view.selectAllChildren();
            else view.clipboardAction(pMsg->wParam);
            return TRUE;
        }
    }
    return CFrameWndEx::PreTranslateMessage(pMsg);
}

void DesignerFrame::OnEnterSizeMove()
{
    // 系统 SC_SIZE / SC_MOVE 循环开始：整窗改成"拉伸上一帧"预览
    m_pane.view().beginSizeMovePreview();
}

void DesignerFrame::OnExitSizeMove()
{
    // 松手：恢复画布子窗口 + 强制一次全量重画，得到精确画面
    m_pane.view().endSizeMovePreview();
}

LRESULT DesignerFrame::OnNcHitTest(CPoint point)
{
    // 无边框但保留系统缩放：四边 6px 直接报 HTLEFT/HTTOP...，
    // 点下去就是系统的 SC_SIZE 循环，手感和原生窗口一致。
    if (::IsZoomed(m_hWnd))
        return HTCLIENT;
    CRect r;
    GetWindowRect(&r);
    const int bw = 6;
    const bool l = point.x < r.left + bw;
    const bool rr = point.x >= r.right - bw;
    const bool t = point.y < r.top + bw;
    const bool b = point.y >= r.bottom - bw;
    if (t && l) return HTTOPLEFT;
    if (t && rr) return HTTOPRIGHT;
    if (b && l) return HTBOTTOMLEFT;
    if (b && rr) return HTBOTTOMRIGHT;
    if (l) return HTLEFT;
    if (rr) return HTRIGHT;
    if (t) return HTTOP;
    if (b) return HTBOTTOM;
    return HTCLIENT;
}

BOOL DesignerFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    hostLog(L"[P1] PreCreateWindow enter");
    BOOL ok = CFrameWndEx::PreCreateWindow(cs);
    hostLog(L"[P2] CFrameWndEx::PreCreateWindow -> %d", (int)ok);

    // 无边框窗体：去掉系统标题栏与边框，标题、最小化/最大化/关闭全部由
    // designer_layout.xml 的 ShellTitle / Role=WinBtn|WinClose 自绘，
    // 于是"设计器自己的外观"也回到"由 XML 完全控制"这条线上。
    // WS_THICKFRAME 保留缩放能力，热区由 DesignerPaneView::tryFrameChrome 处理。
    // 连 WS_THICKFRAME 也去掉：只要留着它，系统就保留"可缩放边框"这一层，
    // DWM 会在客户区内侧描 1px 可见边框、WM_NCHITTEST 也会把边缘判成 HTLEFT/HTRIGHT，
    // 缩放/拖动时还会走系统 SC_SIZE 循环画一圈白框。缩放改由 pane 自己 SetWindowPos 实现。
    // 缩放/拖动交回系统（SC_SIZE / SC_MOVE）。自己 SetWindowPos 每帧改矩形，
    // 布局与重画都挤在鼠标消息里，手感必然发涩、拖影；系统循环由 DWM 拿快照搬整窗，
    // 画多久都不影响跟手。WS_THICKFRAME 只负责提供 HTLEFT/HTRIGHT 这类命中结果，
    // 可见边框仍由 OnNcCalcSize 满矩形 + DWM 属性(33/34/37)压掉。
    cs.style = WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN;
    cs.dwExStyle |= WS_EX_APPWINDOW;    // 无边框窗口也要有任务栏按钮
    return ok;
}

// 客户区铺满整窗：不留给系统标题栏/边框任何厚度
void DesignerFrame::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
    (void)bCalcValidRects;
    if (lpncsp != nullptr)
    {
        lpncsp->rgrc[0] = lpncsp->rgrc[0];   // 保持窗口矩形，不做内缩
    }
    // 不调用基类：基类会按 caption 高度重新算一遍，又把边留回来
}

// 无边框窗口最大化默认会盖住任务栏：按显示器工作区收口
void DesignerFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
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
            lpMMI->ptMinTrackSize.x = 720;
            lpMMI->ptMinTrackSize.y = 480;
        }
    }
    CFrameWndEx::OnGetMinMaxInfo(lpMMI);
}

BOOL DesignerFrame::createFrame(const CRect& rect)
{
    hostLog(L"[1] createFrame enter");
    LPCTSTR cls = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr);
    if (!Create(cls, _T("PyMfc UI Designer"), WS_OVERLAPPEDWINDOW, rect))
        return FALSE;
    hostLog(L"[2] frame Create ok");

    // 停靠宿主要先允许自身停靠，pane 再注册可停靠边，最后 DockPane 落到默认位置。
    EnableDocking(CBRS_ALIGN_ANY);
    hostLog(L"[3] EnableDocking ok");

    if (!m_pane.createPane(this, kId_VIEW_DESIGNER, CRect(0, 0, 1100, 900)))
        return FALSE;
    hostLog(L"[4] pane create ok hwnd=%p", m_pane.GetSafeHwnd());

    m_pane.EnableDocking(CBRS_ALIGN_ANY);
    hostLog(L"[5] pane EnableDocking ok");

    m_pane.SetMinSize(CSize(360, 240));
    m_pane.SetWindowText(_T("界面设计器"));
    // 设计器是框架的主内容面板，默认就铺满客户区：
    // DockPane 传客户区矩形，dock manager 才会把整幅宽度记成停靠宽度
    //（不传 lpRect 时它会取窗格自身矩形，被压成客户区的一半）。
    CRect rcClient;
    GetClientRect(&rcClient);
    DockPane(&m_pane, AFX_IDW_DOCKBAR_LEFT, &rcClient);
    CRect rcPane;
    m_pane.GetWindowRect(&rcPane);
    hostLog(L"[6] DockPane ok client=%dx%d pane=%dx%d", rcClient.Width(), rcClient.Height(),
            rcPane.Width(), rcPane.Height());

    return TRUE;
}

int DesignerFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    hostLog(L"[A] DesignerFrame::OnCreate enter");
    if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
        return -1;
    hostLog(L"[B] CFrameWndEx::OnCreate ok");

    // 设计器是框架的主内容面板，不允许关掉（关掉就只剩空框架）；
    // 浮动/自动隐藏/停靠仍然保留，停靠位置由 DockPane 决定。
    ModifyStyle(0, WS_CLIPCHILDREN);
    stripWindowFrame();
    hideDockSliders();
    // 不需要定时器：滑块只会在布局之后被停靠体系重新显示，
    // RecalcLayout（重写）、OnSize 末尾各调一次 hideDockSliders，并把 hide 排到消息队尾（postChromeFix），
    // 覆盖"MFC 在 OnIdle 里再排一次布局"的情况。空闲时不再周期性碰窗口，完全不产生绘制。
    return 0;
}

// 无边框窗口在 Win10/11 上仍会被 DWM 描一圈 1px 边框、加圆角和投影：
// 关掉非客户区渲染 + 圆角 + 边框色，外壳才真的是 XML 画出来的那一块。
// 用动态加载，不额外给工程加 dwmapi.lib 依赖。
void DesignerFrame::stripWindowFrame()
{
    HMODULE dwm = ::LoadLibraryW(L"dwmapi.dll");
    if (dwm == nullptr) return;
    typedef HRESULT(WINAPI* SetAttr)(HWND, DWORD, LPCVOID, DWORD);
    SetAttr fn = reinterpret_cast<SetAttr>(::GetProcAddress(dwm, "DwmSetWindowAttribute"));
    HRESULT hr[4] = {E_FAIL, E_FAIL, E_FAIL, E_FAIL};
    if (fn != nullptr && m_hWnd != nullptr)
    {
        DWORD doNotRound = 1;        // DWMWA_WINDOW_CORNER_PREFERENCE = DWMWCP_DONOTROUND
        hr[0] = fn(m_hWnd, 33, &doNotRound, sizeof(doNotRound));
        DWORD colorNone = 0xFFFFFFFE; // DWMWA_BORDER_COLOR = DWMWA_COLOR_NONE
        hr[1] = fn(m_hWnd, 34, &colorNone, sizeof(colorNone));
        DWORD thin = 0;              // DWMWA_VISIBLE_FRAME_BORDER_THICKNESS = 0，去掉那圈描边
        hr[2] = fn(m_hWnd, 37, &thin, sizeof(thin));
        // 兜底：整块非客户区渲染交给 DWM 关掉（连失焦时那圈描边一起不再画）
        DWORD ncDisabled = 1;        // DWMWA_NCRENDERING_POLICY = DWMNCRP_DISABLED
        hr[3] = fn(m_hWnd, 2, &ncDisabled, sizeof(ncDisabled));
    }
    ::FreeLibrary(dwm);
    hostLog(L"[dwm] corner=%08lX border=%08lX thickness=%08lX ncrender=%08lX\n",
            (unsigned long)hr[0], (unsigned long)hr[1],
            (unsigned long)hr[2], (unsigned long)hr[3]);
}

// MFC 停靠体系会在面板右缘留一个 4px 的 AFX_SLIDER 滑块窗口：
// 它默认用系统窗口底色（白），正好贴着窗口右边，看起来就像"外壳的一圈外框"；
// 同时它抢走右缘的鼠标，导致左右缩放拖不动。设计器是全窗口停靠，不再需要这个滑块。
// MFC 的停靠体系在每次 RecalcLayout 里都会重建/显示那个 4px 的 AFX_SLIDER，
// 所以只能"布局之后"再收：这是右缘白条反复出现的真正原因。
void DesignerFrame::RecalcLayout(BOOL bNotify)
{
    CFrameWndEx::RecalcLayout(bNotify);
    hideDockSliders();
}

void DesignerFrame::hideDockSliders()
{
    if (m_hWnd == nullptr) return;
    CRect rc;
    GetClientRect(&rc);
    if (rc.Width() <= 0 || rc.Height() <= 0) return;

    HWND controlBar = nullptr;
    HWND slider = nullptr;
    // 递归找：MFC 会把 AFX_SLIDER 挂在停靠条/控制条底下，不再只是框架的直接子窗口。
    // 那个滑块默认用系统窗口底色（白），贴在最右/最左缘，看起来就是"外框的一条白边"。
    struct Ctx { HWND root; HWND bar; HWND sld; } ctx{ m_hWnd, nullptr, nullptr };
    ::EnumChildWindows(m_hWnd, [](HWND h, LPARAM lp) -> BOOL {
        Ctx* c = reinterpret_cast<Ctx*>(lp);
        wchar_t cls[128] = {0};
        ::GetClassNameW(h, cls, 127);
        if (wcsstr(cls, L"Slider") != nullptr) c->sld = h;
        else if (::GetParent(h) == c->root && wcsstr(cls, L"ControlBar") != nullptr) c->bar = h;
        return TRUE;
    }, (LPARAM)&ctx);
    controlBar = ctx.bar;
    slider = ctx.sld;

    // 滑块不只隐藏，还要把它挪到屏幕外并压成 0 尺寸：
    // 停靠体系下一轮 RecalcLayout 会重新 ShowWindow，只隐藏会被它再显示出来。
    // 已经在屏外且是 0 尺寸就不再调 SetWindowPos —— 每次调用都会走一遍窗口重排/重绘。
    if (slider != nullptr)
    {
        RECT sr = {};
        ::GetWindowRect(slider, &sr);
        const bool alreadyParked = (sr.right - sr.left == 0 && sr.bottom - sr.top == 0 &&
                                    sr.left <= -9000);
        if (!alreadyParked)
            ::SetWindowPos(slider, nullptr, -10000, -10000, 0, 0,
                           SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);
    }
    // 停靠条自己被 MFC 留出 4px 给滑块：把它铺满客户区，那 4px 就回到画面上。
    // 只在尺寸真的不对时才改 —— 每 300ms 无条件 SetWindowPos 会触发停靠重排，整窗重画，看起来就是抖。
    if (controlBar != nullptr)
    {
        RECT br = {};
        ::GetWindowRect(controlBar, &br);
        const int bw = br.right - br.left;
        const int bh = br.bottom - br.top;
        const int bx = br.left, by = br.top;
        POINT org = {0, 0};
        ::ClientToScreen(m_hWnd, &org);
        if (bw != rc.Width() || bh != rc.Height() || bx != org.x || by != org.y)
            ::SetWindowPos(controlBar, nullptr, 0, 0, rc.Width(), rc.Height(),
                           SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void DesignerFrame::postChromeFix()
{
    ::PostMessageW(m_hWnd, WM_APP + 100, 0, 0);   // MFC 布局跑完会重新显示滑块，排到队尾再收一次
}

void DesignerFrame::OnClose()
{
    // 编辑器关窗先问一句：有未保存修改时弹“保存 / 不保存 / 取消”
    if (!m_pane.view().confirmClose())
        return;                       // 用户取消，窗口保持打开
    CFrameWndEx::OnClose();
}

void DesignerFrame::OnTimer(UINT_PTR nIDEvent)
{
    CFrameWndEx::OnTimer(nIDEvent);
}

// 激活/失活切换时 DWM 会按新状态重画窗口描边（失焦那一圈白框就是这么冒出来的），
// 每次切换后重新压一遍属性，让它没有可画的边框。
void DesignerFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    CFrameWndEx::OnActivate(nState, pWndOther, bMinimized);
    stripWindowFrame();
}

// 无边框窗口失焦时那圈白框，是 DefWindowProc 在 WM_NCACTIVATE 里按"非激活边框"重画非客户区造成的。
// 我们的非客户区已经被 OnNcCalcSize 压成 0，它没地方画正面，就靠这条消息把状态画到窗口描边上。
// 直接吃掉、不交给默认处理，外壳才完全由 XML 的像素决定。
BOOL DesignerFrame::OnNcActivate(BOOL bActive)
{
    (void)bActive;
    return TRUE;
}

// Win10/11 的 UAH（用户账户控制相关绘制）还会单独发这两条消息补画标题/边框，一并吃掉。
LRESULT DesignerFrame::OnNcUahDraw(WPARAM wParam, LPARAM lParam)
{
    (void)wParam; (void)lParam;
    return 0;
}

BOOL DesignerFrame::OnEraseBkgnd(CDC* pDC)
{
    CRect rc;
    GetClientRect(&rc);
    pDC->FillSolidRect(&rc, RGB(21, 24, 29));
    return TRUE;
}

// 停靠面板的宽度默认是"停靠那一刻"的宽度，窗口最大化/拉伸时不会跟着变，
// 于是右边会留一大块框架空底色。主内容面板（本设计器）必须跟客户区同宽，
// 否则用户看到的就是"设计器缩在左边一条"。浮动状态不受影响，用户仍可拖自由尺寸。
void DesignerFrame::OnSize(UINT nType, int cx, int cy)
{
    CFrameWndEx::OnSize(nType, cx, cy);
    if (nType == SIZE_MINIMIZED)
        return;
    if (m_pane.GetSafeHwnd() == nullptr || m_pane.IsFloating())
        return;

    CRect rc;
    GetClientRect(&rc);
    if (rc.Width() > 0 && rc.Height() > 0)
    {
        m_pane.SetWindowPos(nullptr, 0, 0, rc.Width(), rc.Height(),
                            SWP_NOZORDER | SWP_NOACTIVATE);
        hideDockSliders();
        postChromeFix();
        // 尺寸变化后 DWM 会重算非客户区，把刚设的"无边框"属性重新画回去 —— 这里再压一次
        stripWindowFrame();
    }
}

