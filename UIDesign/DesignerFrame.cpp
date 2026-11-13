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
    ON_WM_NCCALCSIZE()
    ON_WM_GETMINMAXINFO()
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

BOOL DesignerFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    hostLog(L"[P1] PreCreateWindow enter");
    BOOL ok = CFrameWndEx::PreCreateWindow(cs);
    hostLog(L"[P2] CFrameWndEx::PreCreateWindow -> %d", (int)ok);

    // 无边框窗体：去掉系统标题栏与边框，标题、最小化/最大化/关闭全部由
    // designer_layout.xml 的 ShellTitle / Role=WinBtn|WinClose 自绘，
    // 于是"设计器自己的外观"也回到"由 XML 完全控制"这条线上。
    // WS_THICKFRAME 保留缩放能力，热区由 DesignerPaneView::tryFrameChrome 处理。
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
    return 0;
}

void DesignerFrame::OnClose()
{
    // 编辑器关窗先问一句：有未保存修改时弹“保存 / 不保存 / 取消”
    if (!m_pane.view().confirmClose())
        return;                       // 用户取消，窗口保持打开
    CFrameWndEx::OnClose();
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
        m_pane.SetWindowPos(nullptr, 0, 0, rc.Width(), rc.Height(),
                            SWP_NOZORDER | SWP_NOACTIVATE);
}

