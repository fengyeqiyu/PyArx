// PyArx Viewer 2.0 —— C++ + WebView2 宿主
// 原生菜单 + 原生状态栏 + 窗口记忆 + 单实例 + 拖放 + 快捷键
// 网页在 <exe 同级>\web\index.html，通过 https://app.local/ 虚拟主机映射加载。

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <objbase.h>
#include <wrl.h>
#include <WebView2.h>
#include <string>

#pragma comment(lib, "dwmapi.lib")

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

static ComPtr<ICoreWebView2Controller> g_ctrl;
static ComPtr<ICoreWebView2> g_web;
static HWND g_hwnd = nullptr;
static std::wstring g_webDir;
static std::wstring g_exeDir;
static std::wstring g_startPage = L"index.html";
static std::wstring g_status = L"就绪";
static std::wstring g_curDrawing = L"101";
static std::wstring g_curPage = L"raw";
static int g_statusH = 26;
static bool g_fullscreen = false;
static RECT g_savedRect = {0};
static HACCEL g_accel = nullptr;
static HANDLE g_mutex = nullptr;

static const wchar_t* kHost = L"app.local";
static const wchar_t* kCls = L"PyArxViewerWnd";

enum {
    ID_OPEN_JSON = 1001, ID_OPEN_EXPORT, ID_EXIT,
    ID_P_RAW = 1011, ID_P_OV, ID_P_TOPO, ID_P_PLAN,
    ID_REFRESH = 1021, ID_DEVTOOLS, ID_FULLSCREEN, ID_ESC,
    ID_D_101 = 1031, ID_D_GY, ID_D_POW,
    ID_ABOUT = 1041
};

static std::wstring ExeDirOf() {
    wchar_t path[MAX_PATH] = {0};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring s(path);
    size_t p = s.find_last_of(L"\\/");
    return p == std::wstring::npos ? s : s.substr(0, p);
}

static std::wstring UserDataDir() {
    wchar_t* base = nullptr;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &base);
    std::wstring s(base ? base : L"");
    if (base) CoTaskMemFree(base);
    s += L"\\PyArxViewer\\EBWebView";
    return s;
}

static std::wstring IniPath() {
    wchar_t* base = nullptr;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &base);
    std::wstring s(base ? base : L"");
    if (base) CoTaskMemFree(base);
    s += L"\\PyArxViewer";
    CreateDirectoryW(s.c_str(), nullptr);
    return s + L"\\view.ini";
}

static std::wstring ExportDir() {
    std::wstring dir = g_exeDir + L"\\export";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

static void PostUi(const std::wstring& json) {
    if (g_web) g_web->PostWebMessageAsJson(json.c_str());
}

static void SendCurrent() {
    std::wstring j = L"{\"cmd\":\"ui\",\"drawing\":\"" + g_curDrawing +
                     L"\",\"page\":\"" + g_curPage + L"\"}";
    PostUi(j);
}

static void SetStatusText(const std::wstring& s) {
    g_status = s;
    if (g_hwnd) {
        SetWindowTextW(g_hwnd, (L"PyArx Viewer — " + s).c_str());
        RECT rc; GetClientRect(g_hwnd, &rc);
        RECT sb = {0, rc.bottom - g_statusH, rc.right, rc.bottom};
        InvalidateRect(g_hwnd, &sb, FALSE);
    }
}

static void CopySceneToWeb(const std::wstring& src) {
    std::wstring dst = g_webDir + L"\\_open.scene.json";
    if (!CopyFileW(src.c_str(), dst.c_str(), FALSE)) {
        MessageBoxW(g_hwnd, L"复制 scene JSON 到 web 目录失败", L"PyArx Viewer", MB_ICONERROR);
        return;
    }
    if (g_web) {
        g_web->Navigate((std::wstring(L"https://") + kHost +
                         L"/index.html?scene=_open.scene.json").c_str());
    }
}

static void OpenJson() {
    wchar_t file[MAX_PATH] = {0};
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = L"页面/场景 (*.html;*.json)\0*.html;*.json\0HTML 页面 (*.html)\0*.html\0Scene JSON (*.json)\0*.json\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"打开页面或场景";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&ofn)) {
        std::wstring p(file);
        size_t dot = p.find_last_of(L'.');
        std::wstring ext = dot == std::wstring::npos ? L"" : p.substr(dot);
        for (auto& c : ext) c = towlower(c);
        if (ext == L".html") {
            if (g_web) g_web->Navigate((L"file:///" + p).c_str());
        } else {
            CopySceneToWeb(p);
        }
    }
}

static void OpenExportDir() {
    ShellExecuteW(nullptr, L"open", ExportDir().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

static void ToggleFullscreen() {
    if (!g_fullscreen) {
        GetWindowRect(g_hwnd, &g_savedRect);
        MONITORINFO mi = {0};
        mi.cbSize = sizeof(mi);
        GetMonitorInfoW(MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTONEAREST), &mi);
        SetWindowLongW(g_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(g_hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                     mi.rcMonitor.right - mi.rcMonitor.left,
                     mi.rcMonitor.bottom - mi.rcMonitor.top,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        g_fullscreen = true;
    } else {
        LONG style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        SetWindowLongW(g_hwnd, GWL_STYLE, style);
        SetWindowPos(g_hwnd, nullptr, g_savedRect.left, g_savedRect.top,
                     g_savedRect.right - g_savedRect.left,
                     g_savedRect.bottom - g_savedRect.top,
                     SWP_FRAMECHANGED | SWP_NOZORDER);
        g_fullscreen = false;
    }
}

static void Layout() {
    if (!g_hwnd) return;
    RECT rc; GetClientRect(g_hwnd, &rc);
    if (g_ctrl) {
        RECT b = {0, 0, rc.right, rc.bottom - g_statusH};
        if (b.bottom < 0) b.bottom = 0;
        g_ctrl->put_Bounds(b);
    }
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

static HMENU BuildMenu() {
    HMENU m = CreateMenu();

    HMENU f = CreatePopupMenu();
    AppendMenuW(f, MF_STRING, ID_OPEN_JSON, L"打开页面 / 场景…\tCtrl+O");
    AppendMenuW(f, MF_STRING, ID_OPEN_EXPORT, L"打开导出目录");
    AppendMenuW(f, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(f, MF_STRING, ID_EXIT, L"退出");
    AppendMenuW(m, MF_POPUP, (UINT_PTR)f, L"文件");

    HMENU v = CreatePopupMenu();
    AppendMenuW(v, MF_STRING, ID_P_RAW, L"原图");
    AppendMenuW(v, MF_STRING, ID_P_OV, L"识别覆模");
    AppendMenuW(v, MF_STRING, ID_P_TOPO, L"拓扑图");
    AppendMenuW(v, MF_STRING, ID_P_PLAN, L"平面图");
    AppendMenuW(v, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(v, MF_STRING, ID_REFRESH, L"刷新\tCtrl+R");
    AppendMenuW(v, MF_STRING, ID_DEVTOOLS, L"开发者工具\tF12");
    AppendMenuW(v, MF_STRING, ID_FULLSCREEN, L"全屏\tF11");
    AppendMenuW(m, MF_POPUP, (UINT_PTR)v, L"视图");

    HMENU d = CreatePopupMenu();
    AppendMenuW(d, MF_STRING, ID_D_101, L"101# 中试车间配电系统图");
    AppendMenuW(d, MF_STRING, ID_D_GY, L"工艺配电及系统图（GYAP）");
    AppendMenuW(d, MF_STRING, ID_D_POW, L"供电·母线图（变电所）");
    AppendMenuW(m, MF_POPUP, (UINT_PTR)d, L"图纸");

    HMENU h = CreatePopupMenu();
    AppendMenuW(h, MF_STRING, ID_ABOUT, L"关于");
    AppendMenuW(m, MF_POPUP, (UINT_PTR)h, L"帮助");

    return m;
}

static void SaveWindow() {
    if (!g_hwnd || g_fullscreen) return;
    RECT rc; GetWindowRect(g_hwnd, &rc);
    wchar_t buf[64];
    std::wstring ini = IniPath();
    wsprintfW(buf, L"%d", rc.left); WritePrivateProfileStringW(L"win", L"x", buf, ini.c_str());
    wsprintfW(buf, L"%d", rc.top); WritePrivateProfileStringW(L"win", L"y", buf, ini.c_str());
    wsprintfW(buf, L"%d", rc.right - rc.left); WritePrivateProfileStringW(L"win", L"w", buf, ini.c_str());
    wsprintfW(buf, L"%d", rc.bottom - rc.top); WritePrivateProfileStringW(L"win", L"h", buf, ini.c_str());
    wsprintfW(buf, L"%d", IsZoomed(g_hwnd) ? 1 : 0); WritePrivateProfileStringW(L"win", L"max", buf, ini.c_str());
}

static void InitWebView(HWND hwnd) {
    auto onCtrl = Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
        [hwnd](HRESULT hr, ICoreWebView2Controller* ctrl) -> HRESULT {
            if (FAILED(hr) || !ctrl) {
                wchar_t msg[128];
                wsprintfW(msg, L"创建 WebView2 控件失败 (hr=0x%08X)", (unsigned)hr);
                MessageBoxW(hwnd, msg, L"PyArx Viewer", MB_ICONERROR);
                return hr;
            }
            g_ctrl = ctrl;
            ComPtr<ICoreWebView2> web;
            g_ctrl->get_CoreWebView2(&web);
            g_web = web;

            ComPtr<ICoreWebView2Settings> st;
            if (SUCCEEDED(web->get_Settings(&st)) && st) {
                st->put_IsStatusBarEnabled(FALSE);
                st->put_AreDefaultContextMenusEnabled(FALSE);
                st->put_IsZoomControlEnabled(FALSE);
                st->put_AreDevToolsEnabled(TRUE);
            }
            ComPtr<ICoreWebView2_3> web3;
            if (SUCCEEDED(web.As(&web3)) && web3) {
                web3->SetVirtualHostNameToFolderMapping(
                    kHost, g_webDir.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
            } else {
                MessageBoxW(hwnd, L"WebView2 运行时版本过低（需要 ICoreWebView2_3）",
                            L"PyArx Viewer", MB_ICONERROR);
                return E_NOINTERFACE;
            }

            ComPtr<ICoreWebView2Controller3> ctrl3;
            if (SUCCEEDED(g_ctrl.As(&ctrl3)) && ctrl3) {
                ctrl3->put_ShouldDetectMonitorScaleChanges(FALSE);
                ctrl3->put_RasterizationScale((double)GetDpiForWindow(hwnd) / 96.0);
            }
            Layout();

            // 网页 -> 外壳
            web->add_WebMessageReceived(
                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                    [hwnd](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                        LPWSTR json = nullptr;
                        if (FAILED(args->get_WebMessageAsJson(&json)) || !json) return S_OK;
                        std::wstring s(json);
                        CoTaskMemFree(json);
                        if (s.find(L"\"open\"") != std::wstring::npos) {
                            OpenJson();
                        } else if (s.find(L"\"reload\"") != std::wstring::npos) {
                            if (g_web) g_web->Reload();
                        } else if (s.find(L"\"exportDir\"") != std::wstring::npos) {
                            OpenExportDir();
                        } else if (s.find(L"\"status\"") != std::wstring::npos) {
                            size_t p = s.find(L"\"text\":\"");
                            if (p != std::wstring::npos) {
                                size_t q = s.find(L"\"", p + 8);
                                if (q != std::wstring::npos) SetStatusText(s.substr(p + 8, q - p - 8));
                            }
                        }
                        return S_OK;
                    }).Get(),
                nullptr);

            // 加载完成后把当前状态推给网页（用于隐藏网页自带顶栏、同步页签）
            web->add_NavigationCompleted(
                Callback<ICoreWebView2NavigationCompletedEventHandler>(
                    [](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs*) -> HRESULT {
                        SendCurrent();
                        return S_OK;
                    }).Get(),
                nullptr);

            // 下载固定落 exe\export
            ComPtr<ICoreWebView2_4> web4;
            if (SUCCEEDED(web->QueryInterface(IID_PPV_ARGS(&web4))) && web4) {
                web4->add_DownloadStarting(
                    Callback<ICoreWebView2DownloadStartingEventHandler>(
                        [](ICoreWebView2*, ICoreWebView2DownloadStartingEventArgs* args) -> HRESULT {
                            LPWSTR def = nullptr;
                            args->get_ResultFilePath(&def);
                            std::wstring name = def ? def : L"export.png";
                            if (def) CoTaskMemFree(def);
                            size_t p = name.find_last_of(L"\\/");
                            if (p != std::wstring::npos) name = name.substr(p + 1);
                            std::wstring full = ExportDir() + L"\\" + name;
                            args->put_ResultFilePath(full.c_str());
                            args->put_Handled(VARIANT_TRUE);
                            return S_OK;
                        }).Get(),
                    nullptr);
            }

            web->Navigate((std::wstring(L"https://") + kHost + L"/" + g_startPage).c_str());
            g_ctrl->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
            return S_OK;
        });

    auto onEnv = Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
        [onCtrl](HRESULT hr, ICoreWebView2Environment* env) -> HRESULT {
            if (FAILED(hr) || !env) {
                wchar_t msg[128];
                wsprintfW(msg, L"创建 WebView2 环境失败 (hr=0x%08X)\n请确认已安装 Edge WebView2 Runtime",
                          (unsigned)hr);
                MessageBoxW(g_hwnd, msg, L"PyArx Viewer", MB_ICONERROR);
                return hr;
            }
            return env->CreateCoreWebView2Controller(g_hwnd, onCtrl.Get());
        });

    std::wstring udf = UserDataDir();
    CreateCoreWebView2EnvironmentWithOptions(nullptr, udf.c_str(), nullptr, onEnv.Get());
}

static void OnCommand(WPARAM id) {
    switch (id) {
    case ID_OPEN_JSON: OpenJson(); break;
    case ID_OPEN_EXPORT: OpenExportDir(); break;
    case ID_EXIT: DestroyWindow(g_hwnd); break;
    case ID_P_RAW: g_curPage = L"raw"; SendCurrent(); SetStatusText(L"原图"); break;
    case ID_P_OV: g_curPage = L"ov"; SendCurrent(); SetStatusText(L"识别覆模"); break;
    case ID_P_TOPO: g_curPage = L"topo"; SendCurrent(); SetStatusText(L"拓扑图"); break;
    case ID_P_PLAN: g_curPage = L"plan"; SendCurrent(); SetStatusText(L"平面图"); break;
    case ID_REFRESH: if (g_web) g_web->Reload(); break;
    case ID_DEVTOOLS: if (g_web) g_web->OpenDevToolsWindow(); break;
    case ID_FULLSCREEN: ToggleFullscreen(); break;
    case ID_ESC: if (g_fullscreen) ToggleFullscreen(); break;
    case ID_D_101: g_curDrawing = L"101"; SendCurrent(); SetStatusText(L"101# 中试车间配电系统图"); break;
    case ID_D_GY: g_curDrawing = L"gy"; SendCurrent(); SetStatusText(L"工艺配电及系统图（GYAP）"); break;
    case ID_D_POW: g_curDrawing = L"pow"; SendCurrent(); SetStatusText(L"供电·母线图（变电所）"); break;
    case ID_ABOUT:
        MessageBoxW(g_hwnd,
            L"PyArx Viewer 2.0\n\nWebView2 宿主 · 三图纸四页签\n"
            L"原图 / 识别覆模 / 拓扑图 / 平面图\n\n"
            L"F11 全屏  Ctrl+R 刷新  F12 开发者工具",
            L"关于", MB_ICONINFORMATION);
        break;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_COMMAND:
        OnCommand(LOWORD(wp));
        return 0;
    case WM_SIZE:
        Layout();
        return 0;
    case WM_SETFOCUS:
        if (g_ctrl) g_ctrl->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        RECT sb = {0, rc.bottom - g_statusH, rc.right, rc.bottom};
        HBRUSH b = CreateSolidBrush(RGB(11, 16, 22));
        FillRect(dc, &sb, b);
        DeleteObject(b);
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(29, 42, 56));
        HPEN old = (HPEN)SelectObject(dc, pen);
        MoveToEx(dc, 0, sb.top, nullptr);
        LineTo(dc, rc.right, sb.top);
        SelectObject(dc, old);
        DeleteObject(pen);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(130, 158, 184));
        RECT tc = sb;
        tc.left += 10;
        tc.right -= 10;
        DrawTextW(dc, g_status.c_str(), -1, &tc, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DROPFILES: {
        HDROP hd = (HDROP)wp;
        wchar_t path[MAX_PATH] = {0};
        if (DragQueryFileW(hd, 0, path, MAX_PATH)) {
            std::wstring p(path);
            size_t dot = p.find_last_of(L'.');
            std::wstring ext = dot == std::wstring::npos ? L"" : p.substr(dot);
            for (auto& c : ext) c = towlower(c);
            if (ext == L".json") {
                CopySceneToWeb(p);
            } else if (ext == L".html") {
                if (g_web) g_web->Navigate((L"file:///" + p).c_str());
            } else {
                MessageBoxW(hwnd, L"支持拖入 .json（场景）与 .html（页面）", L"PyArx Viewer", MB_ICONINFORMATION);
            }
        }
        DragFinish(hd);
        SetForegroundWindow(hwnd);
        return 0;
    }
    case WM_CLOSE:
        SaveWindow();
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nShow) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) return 1;

    // 单实例：再次启动只把已有窗口提到前台
    g_mutex = CreateMutexW(nullptr, TRUE, L"Local\\PyArxViewer_Single");
    if (g_mutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND h = FindWindowW(kCls, nullptr);
        if (h) {
            ShowWindow(h, IsIconic(h) ? SW_RESTORE : SW_SHOW);
            SetForegroundWindow(h);
        }
        return 0;
    }

    g_exeDir = ExeDirOf();
    g_webDir = g_exeDir + L"\\web";
    if (GetFileAttributesW(g_webDir.c_str()) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(nullptr, L"找不到 web 目录（应与 exe 同级）", L"PyArx Viewer", MB_ICONERROR);
        return 1;
    }

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv && argc > 1) {
        std::wstring a = argv[1];
        size_t p = a.find_last_of(L"\\/");
        std::wstring fn = (p == std::wstring::npos) ? a : a.substr(p + 1);
        if (fn.find(L".html") != std::wstring::npos) g_startPage = fn;
    }
    if (argv) LocalFree(argv);

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.lpszClassName = kCls;
    wc.hbrBackground = CreateSolidBrush(RGB(20, 24, 27));
    RegisterClassExW(&wc);

    RECT r = {0, 0, 1440, 920};
    std::wstring ini = IniPath();
    int x = GetPrivateProfileIntW(L"win", L"x", r.left, ini.c_str());
    int y = GetPrivateProfileIntW(L"win", L"y", r.top, ini.c_str());
    int w = GetPrivateProfileIntW(L"win", L"w", r.right, ini.c_str());
    int h = GetPrivateProfileIntW(L"win", L"h", r.bottom, ini.c_str());
    if (w < 640) w = 1440;
    if (h < 480) h = 920;

    g_hwnd = CreateWindowExW(0, kCls, L"PyArx Viewer — 101# 中试车间配电系统图",
                             WS_OVERLAPPEDWINDOW, x, y, w, h,
                             nullptr, BuildMenu(), hInst, nullptr);
    if (!g_hwnd) return 1;

    DragAcceptFiles(g_hwnd, TRUE);
    ShowWindow(g_hwnd, nShow);
    UpdateWindow(g_hwnd);
    if (GetPrivateProfileIntW(L"win", L"max", 1, ini.c_str())) ShowWindow(g_hwnd, SW_MAXIMIZE);

    DWORD dark = 1;
    COLORREF capCol = RGB(26, 30, 34), txtCol = RGB(230, 232, 235);
    DwmSetWindowAttribute(g_hwnd, 20, &dark, sizeof(dark));
    DwmSetWindowAttribute(g_hwnd, 19, &dark, sizeof(dark));
    DwmSetWindowAttribute(g_hwnd, 35, &capCol, sizeof(capCol));
    DwmSetWindowAttribute(g_hwnd, 36, &txtCol, sizeof(txtCol));

    ACCEL acc[5] = {
        {FVIRTKEY | FCONTROL, 'O', ID_OPEN_JSON},
        {FVIRTKEY | FCONTROL, 'R', ID_REFRESH},
        {FVIRTKEY, VK_F11, ID_FULLSCREEN},
        {FVIRTKEY, VK_F12, ID_DEVTOOLS},
        {FVIRTKEY, VK_ESCAPE, ID_ESC}
    };
    g_accel = CreateAcceleratorTableW(acc, 5);

    InitWebView(g_hwnd);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        if (g_accel && TranslateAcceleratorW(g_hwnd, g_accel, &m)) continue;
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    if (g_accel) DestroyAcceleratorTable(g_accel);
    g_web = nullptr;
    g_ctrl = nullptr;
    CoUninitialize();
    return 0;
}
