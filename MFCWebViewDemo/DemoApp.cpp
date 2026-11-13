// MFCWebView 演示：MFC 只做窗口外壳与两栏布局，两个 WebView2 实例各自渲染自己的页面。
//   侧栏 = 独立 WebView（sidebar.html）：品牌、导航、收起按钮、右缘拖宽手柄
//   主区 = 独立 WebView（demo.html）  ：面包屑、工具条、表单、表格、状态栏
// 两个页面之间不直接互发消息，全部经 MFC 转发（nav 命令 → nav.change 事件）。
#include <afxwin.h>
#include <afxdisp.h>
#include "MFCWebView.h"

namespace
{
    // 从单层 JSON 里取字符串字段（够用即可，复杂解析交给 WebShell）
    std::wstring fieldOf(const std::wstring& json, const wchar_t* key)
    {
        const std::wstring pat = L"\"" + std::wstring(key) + L"\":\"";
        const size_t p = json.find(pat);
        if (p == std::wstring::npos) return L"";
        const size_t b = p + pat.size();
        const size_t q = json.find(L'"', b);
        return q == std::wstring::npos ? std::wstring() : json.substr(b, q - b);
    }

    std::wstring titleOf(const std::wstring& page)
    {
        if (page == L"plan") return L"配电干线图";
        if (page == L"devices") return L"设备台账";
        if (page == L"report") return L"报表中心";
        if (page == L"settings") return L"设置";
        return L"总览";
    }
}

class CDemoFrame : public CFrameWnd
{
    DECLARE_DYNAMIC(CDemoFrame)
public:
    CDemoFrame() = default;

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()

private:
    void layoutPanes();
    void emitPaneState();
    void toggleSidebar();
    void beginSidebarDrag();
    bool onSideCommand(const std::wstring& json);
    bool onMainCommand(const std::wstring& json);

    CWnd m_wndSide;
    CWnd m_wndMain;
    webmfc::WebShell m_side;      // 侧栏：独立 WebView 实例
    webmfc::WebShell m_main;      // 主区：独立 WebView 实例
    int m_sideW = 232;            // 侧栏宽度（像素）
    bool m_sideVisible = true;
    bool m_ready = false;
};

IMPLEMENT_DYNAMIC(CDemoFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CDemoFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

int CDemoFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    const CString cls = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW),
                                           (HBRUSH)::GetStockObject(WHITE_BRUSH), nullptr);
    m_wndSide.CreateEx(0, cls, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                       CRect(0, 0, m_sideW, 100), this, 1000);
    m_wndMain.CreateEx(0, cls, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                       CRect(m_sideW, 0, m_sideW + 100, 100), this, 1001);

    layoutPanes();

    // 原生边框负责改尺寸，所以不需要 web 侧模拟边框
    m_side.setResizeBorder(0);
    m_main.setResizeBorder(0);

    std::wstring err;
    if (!m_side.attach(m_wndSide.m_hWnd, L"", err))
    {
        AfxMessageBox(CString(L"侧栏 WebView 创建失败：") + err.c_str());
        return -1;
    }
    if (!m_main.attach(m_wndMain.m_hWnd, L"", err))
    {
        AfxMessageBox(CString(L"主区 WebView 创建失败：") + err.c_str());
        return -1;
    }

    m_side.setCommandHandler([this](const std::wstring& j) { return onSideCommand(j); });
    m_main.setCommandHandler([this](const std::wstring& j) { return onMainCommand(j); });

    wchar_t exe[MAX_PATH] = { 0 };
    ::GetModuleFileNameW(nullptr, exe, MAX_PATH);
    wchar_t* tail = wcsrchr(exe, L'\\');
    if (tail) tail[1] = 0;
    const std::wstring base(exe);

    m_side.loadPage(base + L"sidebar.html");
    m_main.loadPage(base + L"demo.html");

    m_ready = true;
    return 0;
}

void CDemoFrame::layoutPanes()
{
    if (!::IsWindow(m_wndSide.m_hWnd) || !::IsWindow(m_wndMain.m_hWnd)) return;

    CRect rc;
    GetClientRect(&rc);
    const int w = m_sideVisible ? m_sideW : 0;

    m_wndSide.ShowWindow(m_sideVisible ? SW_SHOWNA : SW_HIDE);
    if (w > 0)
        m_wndSide.MoveWindow(0, 0, w, rc.bottom, TRUE);
    m_wndMain.MoveWindow(w, 0, rc.right - w, rc.bottom, TRUE);
}

void CDemoFrame::emitPaneState()
{
    const std::wstring json = L"{\"event\":\"pane.state\",\"collapsed\":" +
                              std::wstring(m_sideVisible ? L"false" : L"true") +
                              L",\"width\":" + std::to_wstring(m_sideW) + L"}";
    m_side.emit(json);   // 侧栏收起时收不到也无所谓
    m_main.emit(json);   // 主区靠它改按钮文案与芯片
}

void CDemoFrame::toggleSidebar()
{
    m_sideVisible = !m_sideVisible;
    layoutPanes();
    emitPaneState();
}

void CDemoFrame::beginSidebarDrag()
{
    // 侧栏页面在右缘 6px 区按下时走到这里：捕获鼠标，直接用鼠标消息改侧栏宽度
    ::SetCapture(m_hWnd);
    for (;;)
    {
        MSG msg;
        if (!::GetMessageW(&msg, nullptr, 0, 0))
            break;
        if (msg.message == WM_MOUSEMOVE)
        {
            int w = (int)GET_X_LPARAM(msg.lParam);
            if (w < 180) w = 180;
            if (w > 460) w = 460;
            if (w != m_sideW)
            {
                m_sideW = w;
                layoutPanes();
                emitPaneState();
            }
        }
        else if (msg.message == WM_LBUTTONUP || msg.message == WM_CAPTURECHANGED)
        {
            break;
        }
        else if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
        {
            break;
        }
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
    ::ReleaseCapture();
}

bool CDemoFrame::onSideCommand(const std::wstring& json)
{
    const std::wstring cmd = fieldOf(json, L"cmd");
    if (cmd == L"nav")
    {
        const std::wstring page = fieldOf(json, L"page");
        const std::wstring title = titleOf(page);
        m_main.emit(L"{\"event\":\"nav.change\",\"page\":\"" + page + L"\",\"title\":\"" + title + L"\"}");
        return true;
    }
    if (cmd == L"pane.toggle") { toggleSidebar(); return true; }
    if (cmd == L"pane.dragbegin") { beginSidebarDrag(); return true; }
    return false;
}

bool CDemoFrame::onMainCommand(const std::wstring& json)
{
    const std::wstring cmd = fieldOf(json, L"cmd");
    if (cmd == L"pane.toggle") { toggleSidebar(); return true; }
    return false;
}

void CDemoFrame::OnSize(UINT nType, int cx, int cy)
{
    CFrameWnd::OnSize(nType, cx, cy);
    layoutPanes();
}

void CDemoFrame::OnDestroy()
{
    m_side.detach();
    m_main.detach();
    CFrameWnd::OnDestroy();
}

class CDemoApp : public CWinApp
{
public:
    BOOL InitInstance() override;
};

CDemoApp theApp;

BOOL CDemoApp::InitInstance()
{
    // WebView2 要求 COM 初始化在 STA 线程上
    if (!AfxOleInit())
        return FALSE;

    CWinApp::InitInstance();

    CDemoFrame* frame = new CDemoFrame();
    if (!frame->Create(nullptr, L"MFCWebView — HTML 驱动的窗体", WS_OVERLAPPEDWINDOW,
                       CRect(60, 40, 1340, 880)))
        return FALSE;

    m_pMainWnd = frame;
    frame->ShowWindow(SW_SHOW);
    frame->UpdateWindow();
    return TRUE;
}
