// PyArx 对话窗 —— MFC 外壳 + WebView2 内嵌页面
// 复用 viewer 的 WebView2 接入方式（静态 WebView2Loader），页面来自本地 playground：
//   http://127.0.0.1:8765/
// MFC 静态链接（uafxcw.lib + /MT），产物单 exe，无额外 dll。

#define _WIN32_WINNT 0x0A00

#include <afxwin.h>
#include <wrl.h>
#include <WebView2.h>
#include <string>
#include <shlobj.h>

#pragma comment(lib, "dwmapi.lib")

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

static const wchar_t* kUrl = L"http://127.0.0.1:8765/";
static const wchar_t* kTitle = L"PyArx · 本地模型对话";

static std::wstring UserDataDir() {
    wchar_t* base = nullptr;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &base);
    std::wstring s(base ? base : L"");
    if (base) CoTaskMemFree(base);
    s += L"\\PyArxChat";
    CreateDirectoryW(s.c_str(), nullptr);
    s += L"\\EBWebView";
    return s;
}

class CChatFrame : public CFrameWnd {
public:
    ComPtr<ICoreWebView2Controller> m_ctrl;
    ComPtr<ICoreWebView2> m_web;

    CChatFrame() {}

    void LayoutWeb() {
        if (!m_ctrl) return;
        CRect rc;
        GetClientRect(&rc);
        RECT wr = { rc.left, rc.top, rc.right, rc.bottom };
        m_ctrl->put_Bounds(wr);
    }

    void InitWebView() {
        std::wstring ud = UserDataDir();
        auto onEnv = Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this, ud](HRESULT hr, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(hr) || !env) {
                    wchar_t buf[160];
                    wsprintfW(buf, L"创建 WebView2 环境失败 (hr=0x%08X)\n用户数据目录：%s",
                              (unsigned)hr, ud.c_str());
                    AfxMessageBox(buf, MB_ICONERROR);
                    return hr;
                }
                env->CreateCoreWebView2Controller(m_hWnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this](HRESULT hr2, ICoreWebView2Controller* ctrl) -> HRESULT {
                            if (FAILED(hr2) || !ctrl) {
                                wchar_t buf[160];
                                wsprintfW(buf, L"创建 WebView2 控件失败 (hr=0x%08X)", (unsigned)hr2);
                                AfxMessageBox(buf, MB_ICONERROR);
                                return hr2;
                            }
                            m_ctrl = ctrl;
                            ComPtr<ICoreWebView2> web;
                            m_ctrl->get_CoreWebView2(&web);
                            m_web = web;

                            ComPtr<ICoreWebView2Settings> st;
                            if (SUCCEEDED(web->get_Settings(&st)) && st) {
                                st->put_AreDefaultContextMenusEnabled(FALSE);
                                st->put_IsZoomControlEnabled(FALSE);
                                st->put_AreDevToolsEnabled(TRUE);
                                st->put_IsStatusBarEnabled(FALSE);
                            }
                            ComPtr<ICoreWebView2Controller3> c3;
                            if (SUCCEEDED(m_ctrl.As(&c3)) && c3) {
                                c3->put_ShouldDetectMonitorScaleChanges(FALSE);
                                c3->put_RasterizationScale((double)GetDpiForWindow(m_hWnd) / 96.0);
                            }
                            LayoutWeb();
                            m_web->Navigate(kUrl);

                            // 页面加载后聚焦，方便直接打字
                            m_web->add_NavigationCompleted(
                                Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                    [this](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs*) -> HRESULT {
                                        if (m_ctrl) m_ctrl->MoveFocus(
                                            COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
                                        return S_OK;
                                    }).Get(),
                                nullptr);
                            return S_OK;
                        }).Get());
                return S_OK;
            });
        CreateCoreWebView2EnvironmentWithOptions(nullptr, ud.c_str(), nullptr, onEnv.Get());
    }

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lp);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDestroy();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CChatFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

int CChatFrame::OnCreate(LPCREATESTRUCT lp) {
    if (CFrameWnd::OnCreate(lp) == -1) return -1;
    InitWebView();
    return 0;
}

void CChatFrame::OnSize(UINT nType, int cx, int cy) {
    CFrameWnd::OnSize(nType, cx, cy);
    if (nType != SIZE_MINIMIZED) LayoutWeb();
}

void CChatFrame::OnDestroy() {
    m_web.Reset();
    m_ctrl.Reset();
    CFrameWnd::OnDestroy();
}

BOOL CChatFrame::OnEraseBkgnd(CDC* pDC) {
    CRect rc;
    GetClientRect(&rc);
    pDC->FillSolidRect(&rc, RGB(13, 16, 22));
    return TRUE;
}

class CChatApp : public CWinApp {
public:
    virtual BOOL InitInstance();
};

CChatApp theApp;

BOOL CChatApp::InitInstance() {
    if (!CWinApp::InitInstance()) return FALSE;

    // WebView2 需要 STA
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    auto* pFrame = new CChatFrame();
    if (!pFrame->Create(nullptr, kTitle, WS_OVERLAPPEDWINDOW,
                        CRect(0, 0, 980, 760))) {
        delete pFrame;
        return FALSE;
    }
    // 居中
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    pFrame->SetWindowPos(nullptr, (sw - 980) / 2, (sh - 760) / 2, 980, 760,
                         SWP_NOZORDER | SWP_NOACTIVATE);

    m_pMainWnd = pFrame;
    pFrame->ShowWindow(m_nCmdShow);
    pFrame->UpdateWindow();
    return TRUE;
}
