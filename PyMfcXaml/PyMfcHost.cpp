// PyMfcHost.cpp —— class PyMfcHost 的实现（从 PyMfcHost.h 类体内的内联定义迁出）。
// 头里只留声明、嵌套类型、数据成员与宏。改实现改这里，改接口改 PyMfcHost.h。

#include "PyMfcHost.h"
#include "PyMfcPlanGL.h"
#include "XamlSurface.h"

PyMfcHost::~PyMfcHost()
{
        if (m_surface) { delete m_surface; m_surface = nullptr; }
        if (m_bgBrush) ::DeleteObject(m_bgBrush);
        if (m_btnBrush) ::DeleteObject(m_btnBrush);
        if (m_font) ::DeleteObject(m_font);
        for (auto& kv : m_apBrush)
            if (kv.second) ::DeleteObject(kv.second);
        m_apBrush.clear();
    }

void PyMfcHost::PostNcDestroy()
{}


BOOL PyMfcHost::create(HWND parent, const CRect& rect)
{
        ensureNativeClass();
        HWND hwnd = ::CreateWindowExW(
            0, L"PyMfcNativeWindow", L"",
            // 子控件占的区域由父窗口裁掉（自绘皮肤由控件自己在 WM_PAINT 里画，
            // 不再让父窗口跨区域作画 —— 父窗口画子控件区域会让系统反过来使子控件失效，
            // 形成"父画→子失效→子请求父画"的无限重绘循环）。
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            rect.left, rect.top, rect.Width(), rect.Height(),
            parent, nullptr, AfxGetInstanceHandle(), nullptr);
        if (hwnd == nullptr)
            return FALSE;
        SubclassWindow(hwnd);
        ::SetPropW(hwnd, kTabHostProp, reinterpret_cast<HANDLE>(1));
        ensureTabHook();
        setTheme(true);
        ensureSurface();
        return TRUE;
    }

void PyMfcHost::ensureSurface()
{
    if (m_surface != nullptr) return;

    // 环境变量 PYARX_SURFACE：1 = 新 D3D11 表面路径；缺省/0 = 旧 GDI 路径（现状不改）。
    wchar_t v[4] = {};
    if (::GetEnvironmentVariableW(L"PYARX_SURFACE", v, 4) < 1 || v[0] != L'1')
        return;

    CRect rc;
    GetClientRect(&rc);
    IXamlSurface* surface = XamlSurfaceCreateD3D11();
    if (surface == nullptr) return;
    if (!surface->Init(m_hWnd, rc.Width(), rc.Height()))
    {
        delete surface;
        return;
    }
    m_surface = surface;
    m_newSurface = true;

    wchar_t diag[512] = {};
    surface->GetDiagnostics(diag, _countof(diag));
    m_surfaceDiag = diag;
    debugLogf(L"[surface] new surface attached: %ls", diag);
}

void PyMfcHost::setTheme(bool dark)
{
        m_dark = dark;
        if (m_bgBrush) { ::DeleteObject(m_bgBrush); m_bgBrush = nullptr; }
        if (m_btnBrush) { ::DeleteObject(m_btnBrush); m_btnBrush = nullptr; }
        if (m_font) { ::DeleteObject(m_font); m_font = nullptr; }

        m_bg = dark ? RGB(49, 56, 66) : RGB(250, 250, 250);
        m_text = dark ? RGB(235, 235, 235) : RGB(30, 30, 30);
        m_btn = dark ? RGB(62, 70, 80) : RGB(240, 242, 245);
        m_border = dark ? RGB(95, 105, 118) : RGB(200, 205, 210);
        m_bgBrush = ::CreateSolidBrush(m_bg);
        m_btnBrush = ::CreateSolidBrush(m_btn);

        // 字体按 DPI 缩放；GetDC 必须成对 ReleaseDC
        HDC screen = ::GetDC(nullptr);
        const int dpi = screen != nullptr ? ::GetDeviceCaps(screen, LOGPIXELSY) : 96;
        if (screen != nullptr)
            ::ReleaseDC(nullptr, screen);
        const int height = -::MulDiv(10, dpi, 72);
        m_font = ::CreateFontW(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        if (m_hWnd != nullptr)
        {
            for (HWND child = ::GetWindow(m_hWnd, GW_CHILD); child != nullptr;
                 child = ::GetWindow(child, GW_HWNDNEXT))
                ::SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            Invalidate(TRUE);
        }
    }

void PyMfcHost::applyFont(HWND hwnd) const
{
        if (hwnd != nullptr && m_font != nullptr)
            ::SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
    }

LRESULT CALLBACK PyMfcHost::PyMfcTabBodyProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC old = reinterpret_cast<WNDPROC>(::GetPropW(hwnd, kTabOldProcProp));
    HBRUSH brush = reinterpret_cast<HBRUSH>(::GetPropW(hwnd, kTabBrushProp));

    if (msg == WM_ERASEBKGND && brush != nullptr)
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        RECT rc = {};
        ::GetClientRect(hwnd, &rc);
        ::FillRect(hdc, &rc, brush);
        return 1;
    }
    if (msg == WM_PAINT)
    {
        LRESULT result = old != nullptr
            ? ::CallWindowProcW(old, hwnd, msg, wParam, lParam)
            : ::DefWindowProcW(hwnd, msg, wParam, lParam);
        if (brush != nullptr)
        {
            HDC hdc = ::GetDC(hwnd);
            RECT area = {};
            ::GetClientRect(hwnd, &area);
            // TCM_ADJUSTRECT(FALSE)：把客户区收成"页签之外的内容区"
            ::SendMessageW(hwnd, TCM_ADJUSTRECT, FALSE, reinterpret_cast<LPARAM>(&area));
            if (!::IsRectEmpty(&area))
                ::FillRect(hdc, &area, brush);
            ::ReleaseDC(hwnd, hdc);
        }
        return result;
    }
    if (old != nullptr)
        return ::CallWindowProcW(old, hwnd, msg, wParam, lParam);
    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

std::map<HWND, bool>& PyMfcHost::caretBlinkStates()
{
    static std::map<HWND, bool> states;
    return states;
}

bool PyMfcHost::caretBlinkOn(HWND hwnd)
{
    auto it = caretBlinkStates().find(hwnd);
    return it == caretBlinkStates().end() ? true : it->second;
}

void PyMfcHost::ensureSkinBoxClass()
{
    static bool done = false;
    if (done)
        return;
    WNDCLASSEXW wc = {sizeof(WNDCLASSEXW)};
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = ::DefWindowProcW;
    wc.hInstance = AfxGetInstanceHandle();
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kSkinBoxClass;
    ::RegisterClassExW(&wc);
    done = true;
}

std::map<HWND, WNDPROC>& PyMfcHost::skinOldProcs()
{
    static std::map<HWND, WNDPROC> table;
    return table;
}

bool PyMfcHost::isSkinPaintClass(const wchar_t* cls)
{
    // 接管 WM_PAINT、像素完全由皮肤层画的系统类。
    // 保留它们的窗口类：状态与交互（焦点、键盘、IME、滚动、选中、命中测试）不动。
    static const wchar_t* const kSkin[] = {
        // 第一批
        L"msctls_progress32", L"msctls_trackbar32", L"ListBox", L"ComboBox",
        // 第二批：页签、树、表格（含表头）、日期框
        L"SysTabControl32", L"SysTreeView32", L"SysListView32", L"SysHeader32",
        L"SysDateTimePick32",
        // 第三批：日历
        L"SysMonthCal32",
        // 第四批：超链接（文字与下划线由皮肤层画，点击由皮肤过程自己派发 NM_CLICK）
        L"SysLink",
        // 第五批：文本输入控件（系统保留输入、光标、IME，只交出色块与文字）
        L"Edit", L"RICHEDIT50W",
        // 第六批：自建画布窗口（QML 对齐的 Switch / SpinBox / RangeSlider / Busy / PageDots）
        L"PyMfcSkinBox",
    };
    for (const wchar_t* name : kSkin)
    {
        if (_wcsicmp(cls, name) == 0)
            return true;
    }
    return false;
}

void PyMfcHost::requestSkinRepaint(HWND hwnd)
{
    if (hwnd != nullptr)
        ::InvalidateRect(hwnd, nullptr, FALSE);
}

bool PyMfcHost::isNativePaintClass(const wchar_t* cls)
{
    static const wchar_t* const kNative[] = {
        L"msctls_progress32", L"msctls_trackbar32", L"ListBox", L"SysListView32",
        L"SysTreeView32", L"SysTabControl32", L"SysDateTimePick32",
        L"SysMonthCal32", L"SysLink", L"RICHEDIT50W", L"Edit",
    };
    // ComboBox 不在这张表里：框身由皮肤层画（drawCombo），这样"展开着的时候
    // 框里那行字"可以由我们钉住——系统原生绘制会跟着鼠标划过的那一项变，
    // 用户明确要求悬浮只高亮列表项、点选才改框里的值。
    // 下拉列表（ComboLBox）仍是系统窗口，高亮照旧。
    for (const wchar_t* name : kNative)
    {
        if (_wcsicmp(cls, name) == 0)
            return true;
    }
    return false;
}

// 分割条把"拖动"整个当成**控件自己的状态机**：
//   down 记下自己的左边界 + 光标屏幕位置；move 只按屏幕位移推出"我现在应该站哪"；
//   对外只报一个数——**分隔条左边界在父客户区里的 x**（绝对位置）。
// 这样上层不必知道坐标系、不必记按下时的基准、更不必做差分：
// 每次回调给的都是"当前手势要求的最终位置"，重排多少次都不会累积误差。
// 之前把控件的客户坐标（或屏幕坐标）交给上层去差分，本质上就是把控件的内部状态
// 丢给了使用者，拖动中控件移位就会漂——这是控件自身的逻辑缺陷，不是调用方的问题。
struct DragBarDrag
{
    bool  active = false;
    POINT downScreen = { 0, 0 };   // 按下时的光标屏幕位置
    int   downLeft = 0;            // 按下时控件左边界（父客户坐标）
    int   parentW = 0;             // 按下时父客户区宽度（拖动中不变）
    int   barW = 0;                // 控件自身宽度
};

static std::map<HWND, DragBarDrag>& dragBarState()
{
    static std::map<HWND, DragBarDrag> s_map;
    return s_map;
}

// 手势 → 目标左边界（父客户坐标）。钳在父客户区里，控件永远拖不出窗口。
static int DragBarTargetLeft(HWND hwnd, LPARAM lParam, const DragBarDrag& st)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    ::ClientToScreen(hwnd, &pt);
    int left = st.downLeft + (pt.x - st.downScreen.x);
    const int maxLeft = st.parentW - st.barW;
    if (left < 0)
        left = 0;
    if (left > maxLeft)
        left = maxLeft;
    return left;
}

LRESULT CALLBACK PyMfcHost::PyMfcDragBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PyMfcHost* host = reinterpret_cast<PyMfcHost*>(::GetPropW(hwnd, L"PyMfcSkinHost"));
    const int id = ::GetDlgCtrlID(hwnd);
    switch (msg)
    {
    case WM_NCHITTEST:
        return HTCLIENT;                            // 否则父窗会把鼠标都吃掉
    case WM_SETCURSOR:
        ::SetCursor(::LoadCursorW(nullptr, IDC_SIZEWE));   // 竖分割：左右箭头
        return TRUE;
    case WM_LBUTTONDOWN:
    {
        ::SetCapture(hwnd);
        DragBarDrag st;
        st.active = true;
        st.downScreen.x = GET_X_LPARAM(lParam);
        st.downScreen.y = GET_Y_LPARAM(lParam);
        ::ClientToScreen(hwnd, &st.downScreen);
        RECT wr;
        ::GetWindowRect(hwnd, &wr);
        st.barW = wr.right - wr.left;
        HWND parent = ::GetParent(hwnd);
        POINT po = { 0, 0 };
        if (parent != nullptr)
            ::ClientToScreen(parent, &po);
        RECT prc = { 0, 0, 0, 0 };
        if (parent != nullptr)
            ::GetClientRect(parent, &prc);
        st.downLeft = wr.left - po.x;
        st.parentW = prc.right - prc.left;
        dragBarState()[hwnd] = st;
        if (host != nullptr)
            host->fireDrag(id, "down", st.downLeft);
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        if (::GetCapture() != hwnd || host == nullptr)
            return 0;
        auto it = dragBarState().find(hwnd);
        if (it == dragBarState().end() || !it->second.active)
            return 0;
        host->fireDrag(id, "move", DragBarTargetLeft(hwnd, lParam, it->second));
        return 0;
    }
    case WM_LBUTTONUP:
    {
        int left = 0;
        auto it = dragBarState().find(hwnd);
        if (it != dragBarState().end() && it->second.active)
        {
            left = DragBarTargetLeft(hwnd, lParam, it->second);
            it->second.active = false;
        }
        if (host != nullptr)
            host->fireDrag(id, "up", left);
        if (::GetCapture() == hwnd)
            ::ReleaseCapture();
        return 0;
    }
    default:
        break;
    }
    auto it = skinOldProcs().find(hwnd);
    WNDPROC old = (it != skinOldProcs().end()) ? it->second
                                                    : ::DefWindowProcW;
    return ::CallWindowProcW(old, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK PyMfcHost::PyMfcOwnerDrawProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Static / SysLink 的类过程默认把 WM_NCHITTEST 回成 HTTRANSPARENT —— 意思是
    // "这点我透明，把点击交给下面的窗口"。我们用 SS_OWNERDRAW 自绘后没覆盖这条，
    // 于是真实鼠标全部落到父窗：卡片点不中、滚轮也滚不到；而 SendMessage 直接投递
    // 绕过命中链，所以模拟点击一直是好的（这个反差就是根因线索）。
    // 自绘控件的交互都自己处理，一律声明可点的客户区。
    if (msg == WM_NCHITTEST)
        return HTCLIENT;

    // 配电画布先接管鼠标：它要自己处理滚轮缩放与拖动平移，不能再转给 Static 的类过程
    if (msg == WM_MOUSEWHEEL || msg == WM_LBUTTONDOWN || msg == WM_MOUSEMOVE ||
        msg == WM_LBUTTONUP || msg == WM_LBUTTONDBLCLK || msg == WM_SETCURSOR ||
        msg == WM_MOUSELEAVE || msg == WM_CAPTURECHANGED)
    {
        HANDLE prop = ::GetPropW(hwnd, kPlanHostProp);
        if (prop != nullptr)
        {
            bool handled = false;
            const LRESULT r = PyMfcPlanCanvasMouse(
                reinterpret_cast<PyMfcHost*>(prop), hwnd, msg, wParam, lParam, handled);
            if (handled)
                return r;
        }
    }

    // 悬浮光标：按钮/卡片这类 owner-draw 控件给手型，空白容器仍是箭头
    if (msg == WM_SETCURSOR)
    {
        if (LOWORD(lParam) == HTCLIENT)
        {
            PyMfcHost* host = reinterpret_cast<PyMfcHost*>(::GetPropW(hwnd, kSkinHostProp));
            if (host != nullptr)
            {
                HCURSOR cur = host->hoverCursor(hwnd);
                if (cur != nullptr)
                {
                    ::SetCursor(cur);
                    TRACKMOUSEEVENT tme = {};
                    tme.cbSize = sizeof(tme);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = hwnd;
                    ::TrackMouseEvent(&tme);
                    return TRUE;
                }
            }
        }
    }
    if (msg == WM_MOUSELEAVE)
        ::SetCursor(::LoadCursorW(nullptr, IDC_ARROW));

    if (msg == WM_SIZE)
    {
        HANDLE szProp = ::GetPropW(hwnd, kPlanHostProp);        if (szProp != nullptr)
        {
            reinterpret_cast<PyMfcHost*>(szProp)->fitPlanCanvas(::GetDlgCtrlID(hwnd));
        }
    }
    if (msg == WM_PAINT)
    {
        wchar_t paintCls[64] = {};
        ::GetClassNameW(hwnd, paintCls, 64);
        if (isNativePaintClass(paintCls))
        {
            // 原生控件：交回原类过程，让它按自己的内容画（绿条/列表项/页签/日期…）
            WNDPROC old = reinterpret_cast<WNDPROC>(::GetPropW(hwnd, kOldProcProp));
            if (old != nullptr)
                return ::CallWindowProcW(old, hwnd, msg, wParam, lParam);
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }
    }
    if (msg == WM_ERASEBKGND)
    {
        // 原生控件（SysListView32 等）自己擦背景，且带 LVS_EX_DOUBLEBUFFER 双缓冲。
        // 若这里用父背景刷整块填满，每次选中/滚动都先铺一遍白底再逐行画回去，
        // 屏幕上就是"选中一行时整块闪一下、旧行还留着"的拖影 —— 交回原生过程。
        wchar_t eraseCls[64] = {};
        ::GetClassNameW(hwnd, eraseCls, 64);
        if (isNativePaintClass(eraseCls))
        {
            WNDPROC oldProc = reinterpret_cast<WNDPROC>(::GetPropW(hwnd, kOldProcProp));
            if (oldProc != nullptr)
                return ::CallWindowProcW(oldProc, hwnd, msg, wParam, lParam);
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        // 配电画布整块自己铺底（drawPlanCanvas 第一件事就是 fillRect 满客户区），
        // 拖动/缩放时每帧先擦一遍父背景再画，屏幕上就是整块画布白闪一下。
        // 画布自己会覆盖全部像素，所以这里声明"已擦除"即可，不填任何东西。
        if (::GetPropW(hwnd, kPlanHostProp) != nullptr)
            return 1;
        // 自绘控件也不再在这里填父背景：填充已经挪进 WM_PAINT 的离屏缓冲里，
        // 和内容一笔合成后整块 BitBlt。分两步做（先擦成底色、再画内容）在拖动/
        // 连续刷新时就是肉眼看到的闪烁。
        return 1;
    }
    if (msg == WM_PRINTCLIENT)
    {
        // PrintWindow 抓图路径：自绘控件在这里按同样的 WM_DRAWITEM 路径画到外部 HDC，
        // 所以 D2D 画布也能正常被抓到（不再是一张黑图）。
        HDC hdc = reinterpret_cast<HDC>(wParam);
        if (hdc == nullptr)
            return 0;
        HWND parent = ::GetParent(hwnd);
        wchar_t clsName[64] = {};
        ::GetClassNameW(hwnd, clsName, 64);
        DRAWITEMSTRUCT dis = {};
        const bool isStatic = (_wcsicmp(clsName, L"Static") == 0);
        dis.CtlType = isStatic ? ODT_STATIC : ODT_BUTTON;
        dis.CtlID = static_cast<UINT>(::GetDlgCtrlID(hwnd));
        dis.itemAction = ODA_DRAWENTIRE;
        dis.itemState = ::IsWindowEnabled(hwnd) ? 0 : ODS_DISABLED;
        dis.hwndItem = hwnd;
        dis.hDC = hdc;
        ::GetClientRect(hwnd, &dis.rcItem);
        if (parent != nullptr)
            ::SendMessageW(parent, WM_DRAWITEM, dis.CtlID, reinterpret_cast<LPARAM>(&dis));
        return 0;
    }
    if (msg == WM_PAINT)
    {
        // 注意：拖拽分隔条这类"既是装饰又是命中热区"的 HWND 仍保留为覆盖层（S2 共存前提），
        // 其自绘链照旧；节点只在没有子 HWND 覆盖的区域出帧。这里不做 WM_PAINT 短路。
        PAINTSTRUCT ps = {};
        HDC hdc = ::BeginPaint(hwnd, &ps);
        HWND parent = ::GetParent(hwnd);
        wchar_t clsName[64] = {};
        ::GetClassNameW(hwnd, clsName, 64);
        DRAWITEMSTRUCT dis = {};
        const bool isStatic = (_wcsicmp(clsName, L"Static") == 0);
        dis.CtlType = isStatic ? ODT_STATIC : ODT_BUTTON;
        dis.CtlID = static_cast<UINT>(::GetDlgCtrlID(hwnd));
        dis.itemID = 0;
        dis.itemAction = ODA_DRAWENTIRE;
        dis.itemState = ::IsWindowEnabled(hwnd) ? 0 : ODS_DISABLED;
        if (!isStatic)
        {
            // 按下/焦点状态由按钮类过程维护，自绘要读出来同步，否则点下去没有视觉反馈
            const LRESULT st = ::SendMessageW(hwnd, BM_GETSTATE, 0, 0);
            if ((st & BST_PUSHED) != 0)
                dis.itemState |= ODS_SELECTED;
            if ((st & BST_FOCUS) != 0)
                dis.itemState |= ODS_FOCUS;
        }
        dis.hwndItem = hwnd;
        ::GetClientRect(hwnd, &dis.rcItem);

        // 双缓冲：内容先画到一张离屏 DIB，"父背景 + 内容"一次合成，最后整块 BitBlt。
        // 原来是先擦成父背景、再让下面这段把内容一笔笔画到窗口 DC 上——中间态在
        // 拖动（画布每帧重画）和面板/状态栏连续刷新时就是屏幕上的闪烁。
        static HDC s_odDc = nullptr;
        static HBITMAP s_odBmp = nullptr;
        static int s_odW = 0, s_odH = 0;
        const int needW = dis.rcItem.right - dis.rcItem.left;
        const int needH = dis.rcItem.bottom - dis.rcItem.top;
        HDC renderDc = hdc;
        if (needW > 0 && needH > 0)
        {
            if (s_odDc == nullptr || s_odW < needW || s_odH < needH)
            {
                const int nw = needW > s_odW ? needW : s_odW;
                const int nh = needH > s_odH ? needH : s_odH;
                if (s_odBmp != nullptr) { ::DeleteObject(s_odBmp); s_odBmp = nullptr; }
                if (s_odDc != nullptr) { ::DeleteDC(s_odDc); s_odDc = nullptr; }
                BITMAPINFO bi = {};
                bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bi.bmiHeader.biWidth = nw;
                bi.bmiHeader.biHeight = -nh;   // 自顶向下，和屏幕/D2D 坐标一致
                bi.bmiHeader.biPlanes = 1;
                bi.bmiHeader.biBitCount = 32;
                bi.bmiHeader.biCompression = BI_RGB;
                void* bits = nullptr;
                HBITMAP bmp = ::CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
                HDC dc = ::CreateCompatibleDC(nullptr);
                if (bmp != nullptr && dc != nullptr)
                {
                    ::SelectObject(dc, bmp);
                    s_odBmp = bmp;
                    s_odDc = dc;
                    s_odW = nw;
                    s_odH = nh;
                }
                else
                {
                    if (bmp != nullptr) ::DeleteObject(bmp);
                    if (dc != nullptr) ::DeleteDC(dc);
                }
            }
            if (s_odDc != nullptr && s_odW >= needW && s_odH >= needH)
                renderDc = s_odDc;
        }

        dis.hDC = renderDc;
        if (renderDc != hdc)
        {
            // 圆角控件边缘之外要露出父背景：填充放在离屏缓冲里，和内容同一次输出
            RECT rc = dis.rcItem;
            if (parent != nullptr)
            {
                HBRUSH br = reinterpret_cast<HBRUSH>(
                    ::SendMessageW(parent, WM_CTLCOLORSTATIC,
                                   reinterpret_cast<WPARAM>(renderDc), reinterpret_cast<LPARAM>(hwnd)));
                ::FillRect(renderDc, &rc, (br != nullptr) ? br : ::GetSysColorBrush(COLOR_BTNFACE));
            }
        }
        if (parent != nullptr)
            ::SendMessageW(parent, WM_DRAWITEM, dis.CtlID, reinterpret_cast<LPARAM>(&dis));
        if (renderDc != hdc)
            ::BitBlt(hdc, 0, 0, needW, needH, renderDc, 0, 0, SRCCOPY);
        ::EndPaint(hwnd, &ps);
        return 0;
    }
    WNDPROC old = reinterpret_cast<WNDPROC>(::GetPropW(hwnd, kOldProcProp));
    if (old != nullptr)
        return ::CallWindowProcW(old, hwnd, msg, wParam, lParam);
    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

void PyMfcHost::ensureWicCom()
{
    static bool once = false;
    if (once)
        return;
    once = true;
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
}

HBITMAP PyMfcHost::loadBitmapFromFile(const std::wstring& path)
{
    if (path.empty())
        return nullptr;
    ensureWicCom();
    IWICImagingFactory* factory = nullptr;
    HRESULT hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory));
    if (FAILED(hr) || factory == nullptr)
        return nullptr;

    HBITMAP result = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* conv = nullptr;
    hr = factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                            WICDecodeMetadataCacheOnDemand, &decoder);
    if (SUCCEEDED(hr) && decoder != nullptr)
        hr = decoder->GetFrame(0, &frame);
    if (SUCCEEDED(hr) && frame != nullptr)
        hr = factory->CreateFormatConverter(&conv);
    if (SUCCEEDED(hr) && conv != nullptr)
        hr = conv->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
                              nullptr, 0.0, WICBitmapPaletteTypeCustom);
    if (SUCCEEDED(hr) && conv != nullptr)
    {
        UINT w = 0, h = 0;
        if (SUCCEEDED(conv->GetSize(&w, &h)) && w > 0 && h > 0)
        {
            BITMAPINFO bi = {};
            bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bi.bmiHeader.biWidth = static_cast<LONG>(w);
            bi.bmiHeader.biHeight = -static_cast<LONG>(h);   // top-down
            bi.bmiHeader.biPlanes = 1;
            bi.bmiHeader.biBitCount = 32;
            bi.bmiHeader.biCompression = BI_RGB;
            void* bits = nullptr;
            HDC screen = ::GetDC(nullptr);
            HBITMAP dib = ::CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
            ::ReleaseDC(nullptr, screen);
            if (dib != nullptr && bits != nullptr)
            {
                const UINT stride = w * 4;
                if (SUCCEEDED(conv->CopyPixels(nullptr, stride, stride * h,
                                               static_cast<BYTE*>(bits))))
                    result = dib;
                else
                    ::DeleteObject(dib);
            }
        }
    }
    if (conv != nullptr)
        conv->Release();
    if (frame != nullptr)
        frame->Release();
    if (decoder != nullptr)
        decoder->Release();
    factory->Release();
    return result;
}

int PyMfcHost::propInt(HWND h, const wchar_t* name, int dflt)
{
    HANDLE v = ::GetPropW(h, name);
    return v != nullptr ? static_cast<int>(reinterpret_cast<std::intptr_t>(v)) : dflt;
}

HWND PyMfcHost::pyMfcNextTabItem(HWND focus, bool back)
{
    HWND parent = ::GetParent(focus);
    if (parent == nullptr)
        return nullptr;
    struct Item
    {
        int tab = 0;
        int seq = 0;
        HWND h = nullptr;
    };
    std::vector<Item> items;
    int order = 0;
    for (HWND h = ::GetWindow(parent, GW_CHILD); h != nullptr;
         h = ::GetWindow(h, GW_HWNDNEXT))
    {
        ++order;
        if ((::GetWindowLongPtrW(h, GWL_STYLE) & WS_TABSTOP) == 0)
            continue;
        if (!::IsWindowVisible(h) || !::IsWindowEnabled(h))
            continue;
        Item it;
        it.tab = propInt(h, L"PyMfcTabIndex", 0x7FFFFFFF);
        it.seq = propInt(h, L"PyMfcSeq", order);
        it.h = h;
        items.push_back(it);
    }
    if (items.empty())
        return nullptr;
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
        if (a.tab != b.tab)
            return a.tab < b.tab;
        return a.seq < b.seq;
    });
    int cur = -1;
    for (int i = 0; i < static_cast<int>(items.size()); ++i)
        if (items[i].h == focus) { cur = i; break; }
    const int n = static_cast<int>(items.size());
    int next;
    if (cur < 0)
        next = back ? n - 1 : 0;
    else
        next = back ? (cur + n - 1) % n : (cur + 1) % n;
    return items[next].h != focus ? items[next].h : nullptr;
}

LRESULT CALLBACK PyMfcHost::PyMfcTabMsgHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && (wParam == PM_REMOVE || wParam == PM_NOREMOVE))
    {
        MSG* msg = reinterpret_cast<MSG*>(lParam);
        if (msg != nullptr && msg->message == WM_KEYDOWN && msg->wParam == VK_TAB &&
            (::GetKeyState(VK_CONTROL) & 0x8000) == 0 &&
            (::GetKeyState(VK_MENU) & 0x8000) == 0)
        {
            HWND focus = ::GetFocus();
            if (focus != nullptr && focus == msg->hwnd)
            {
                HWND cur = focus;
                bool ours = false;
                while (cur != nullptr)
                {
                    if (::GetPropW(cur, kTabHostProp) != nullptr)
                    {
                        ours = true;
                        break;
                    }
                    cur = ::GetParent(cur);
                }
                if (ours)
                {
                    // Shift 用全局状态判断：合成输入（keybd_event）不改本线程的
                    // GetKeyState，只改全局键状态，用 GetKeyState 会永远认为"没按 Shift"
                    const BOOL back = (::GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                    HWND next = pyMfcNextTabItem(focus, back != 0);
                    if (next != nullptr && next != focus)
                        ::SetFocus(next);
                    // 吃掉这条按键：否则控件自己处理会响一声，焦点也不会有反馈
                    msg->message = WM_NULL;
                    msg->wParam = 0;
                    msg->lParam = 0;
                }
            }
        }
        // 悬浮滚轮：WM_MOUSEWHEEL 只发给焦点窗口，鼠标压在别的滚动条上就滚不动。
        // 这里按"光标下的控件"分流，命中滚动条/滚动区域条带就把消息吃掉。
        else if (msg != nullptr && msg->message == WM_MOUSEWHEEL)
        {
            POINT pt = {};
            pt.x = static_cast<LONG>(static_cast<short>(LOWORD(msg->lParam)));
            pt.y = static_cast<LONG>(static_cast<short>(HIWORD(msg->lParam)));
            const int notches = GET_WHEEL_DELTA_WPARAM(msg->wParam) / WHEEL_DELTA;
            if (notches != 0 && PyMfcHost::wheelHoverAtPoint(pt, notches))
            {
                msg->message = WM_NULL;
                msg->wParam = 0;
                msg->lParam = 0;
            }
        }
    }
    return ::CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void PyMfcHost::ensureTabHook()
{
    if (g_tabHook == nullptr)
        g_tabHook = ::SetWindowsHookExW(WH_GETMESSAGE, &PyMfcTabMsgHook, nullptr,
                                        ::GetCurrentThreadId());
}

void PyMfcHost::splitterNeighbors(HWND self, HWND* above, HWND* below)
{
    *above = nullptr;
    *below = nullptr;
    RECT me = {};
    ::GetWindowRect(self, &me);
    HWND parent = ::GetParent(self);
    if (parent == nullptr)
        return;
    // 容差要覆盖布局间距：StackPanel Spacing=8 时两块之间本来就隔着 8px，
    // 只容忍 4px 就会"明明紧挨着却找不到邻居"，拖了也带不动别人
    long bestAbove = 17, bestBelow = 17;
    // 遍历的是父窗口的兄弟控件；以前写成 GetWindow(self, GW_CHILD)，
    // 那是分隔条自己的子窗口（它一个子窗口都没有），所以邻居恒为空、永远拖不动别人
    for (HWND child = ::GetWindow(parent, GW_CHILD); child != nullptr;
         child = ::GetWindow(child, GW_HWNDNEXT))
    {
        if (child == self || !::IsWindowVisible(child))
            continue;
        RECT rc = {};
        ::GetWindowRect(child, &rc);
        const long overlap = (std::min)(me.right, rc.right) - (std::max)(me.left, rc.left);
        if (overlap <= 20)
            continue;
        const long dAbove = labs(me.top - rc.bottom);
        const long dBelow = labs(rc.top - me.bottom);
        if (dAbove < bestAbove)
        {
            bestAbove = dAbove;
            *above = child;
        }
        if (dBelow < bestBelow)
        {
            bestBelow = dBelow;
            *below = child;
        }
    }
}

LRESULT CALLBACK PyMfcHost::PyMfcSplitterProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND s_above = nullptr;
    static HWND s_below = nullptr;
    static POINT s_start = {};
    static RECT s_aboveRect = {};
    static RECT s_belowRect = {};
    static RECT s_selfRect = {};

    switch (msg)
    {
    case WM_LBUTTONDOWN:
    {
        ::SetCapture(hwnd);
        ::GetCursorPos(&s_start);
        splitterNeighbors(hwnd, &s_above, &s_below);
        ::GetWindowRect(hwnd, &s_selfRect);
        if (s_above != nullptr)
            ::GetWindowRect(s_above, &s_aboveRect);
        if (s_below != nullptr)
            ::GetWindowRect(s_below, &s_belowRect);
        // SetWindowPos 对子窗口要的是"父窗口客户区坐标"，而 GetWindowRect 返回的是
        // 屏幕坐标：不换算的话一拖就跑到屏幕外，看起来像"拖不动"
        HWND parent = ::GetParent(hwnd);
        if (parent != nullptr)
        {
            ::MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&s_selfRect), 2);
            if (s_above != nullptr)
                ::MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&s_aboveRect), 2);
            if (s_below != nullptr)
                ::MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&s_belowRect), 2);
        }
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        if (::GetCapture() == hwnd)
        {
            POINT pt = {};
            ::GetCursorPos(&pt);
            long dy = pt.y - s_start.y;
            if (dy == 0)
                return 0;

            // 拖动量要裁：自身不能挪出父窗口客户区上边；上面邻居不能被压到 24px 以下；
            // 下面邻居也不能被压到 24px 以下
            HWND parent = ::GetParent(hwnd);
            if (dy < -s_selfRect.top)
                dy = -s_selfRect.top;
            if (s_above != nullptr)
            {
                const long maxDy = (s_aboveRect.bottom - s_aboveRect.top) - 24;
                if (dy > maxDy)
                    dy = maxDy;
            }
            if (s_below != nullptr)
            {
                const long minDy = -((s_belowRect.bottom - s_belowRect.top) - 24);
                if (dy < minDy)
                    dy = minDy;
            }
            if (dy == 0)
                return 0;

            ::SetWindowPos(hwnd, nullptr, s_selfRect.left, s_selfRect.top + dy,
                           s_selfRect.right - s_selfRect.left,
                           s_selfRect.bottom - s_selfRect.top,
                           SWP_NOZORDER | SWP_NOACTIVATE);
            if (s_above != nullptr)
            {
                // 上面那块：顶边不动，底边跟着分隔条，高度随之增减
                ::SetWindowPos(s_above, nullptr, s_aboveRect.left, s_aboveRect.top,
                               s_aboveRect.right - s_aboveRect.left,
                               (s_aboveRect.bottom - s_aboveRect.top) + dy,
                               SWP_NOZORDER | SWP_NOACTIVATE);
            }
            if (s_below != nullptr)
            {
                // 下面那块：底边不动，顶边跟着分隔条
                const long nTop = s_belowRect.top + dy;
                ::SetWindowPos(s_below, nullptr, s_belowRect.left, nTop,
                               s_belowRect.right - s_belowRect.left,
                               s_belowRect.bottom - nTop,
                               SWP_NOZORDER | SWP_NOACTIVATE);
            }
            if (parent != nullptr)
                ::InvalidateRect(parent, nullptr, TRUE);
            return 0;
        }
        break;
    }
    case WM_LBUTTONUP:
    {
        if (::GetCapture() == hwnd)
            ::ReleaseCapture();
        s_above = nullptr;
        s_below = nullptr;
        ::InvalidateRect(::GetParent(hwnd), nullptr, TRUE);
        return 0;
    }
    case WM_SETCURSOR:
        ::SetCursor(::LoadCursorW(nullptr, IDC_SIZENS));
        return TRUE;
    default:
        break;
    }
    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

// 控件级事件子类化：只负责"看见"并把事件转给宿主，随后原样转发给上一层窗口过程。
// 不带任何绘制/状态改动，所以不会和 owner-draw、皮肤过程打架。
static LRESULT CALLBACK PyMfcEventSubProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    PyMfcHost* host = reinterpret_cast<PyMfcHost*>(::GetPropW(hwnd, PyMfcHost::kEvtHostProp));
    if (host != nullptr)
    {
        const int id = static_cast<int>(::GetDlgCtrlID(hwnd));
        switch (msg)
        {
        case WM_LBUTTONDOWN: host->fireControlEvent(id, L"mousedown"); break;
        case WM_LBUTTONUP:   host->fireControlEvent(id, L"mouseup");   break;
        case WM_MOUSEMOVE:   host->fireControlEvent(id, L"mousemove"); break;
        case WM_KEYDOWN:     host->fireControlEvent(id, L"keydown");   break;
        case WM_KEYUP:       host->fireControlEvent(id, L"keyup");     break;
        case WM_SETFOCUS:    host->fireControlEvent(id, L"gotfocus");  break;
        case WM_KILLFOCUS:   host->fireControlEvent(id, L"lostfocus"); break;
        default: break;
        }
    }
    WNDPROC prev = reinterpret_cast<WNDPROC>(::GetPropW(hwnd, PyMfcHost::kEvtPrevProcProp));
    if (prev != nullptr)
        return ::CallWindowProcW(prev, hwnd, msg, wp, lp);
    return ::DefWindowProcW(hwnd, msg, wp, lp);
}

HWND PyMfcHost::createControl(const wchar_t* cls, const std::wstring& text,
                       DWORD style, int x, int y, int w, int h, int id)
{
        AFX_MANAGE_STATE(AfxGetAppModuleState());

        // 通知只能经 ON_COMMAND_RANGE(kFirstId, kLastId) 路由，
        // 低位 ID（如 101）建得出来但回调永远不会触发，必须显式拒绝。
        if (id < static_cast<int>(kFirstId) || id > static_cast<int>(kLastId))
            throw std::invalid_argument("PyMfc 控件 ID 必须在 1000..65000 之间");

        // 复用页面时会重复 add_*：同 ID 的旧控件先销毁，避免出现两个同 ID 控件
        if (m_hWnd != nullptr)
        {
            if (HWND old = ::GetDlgItem(m_hWnd, id))
            {
                ::SetWindowLongPtrW(old, GWLP_ID, 0);
                ::DestroyWindow(old);
            }
        }

        if (_wcsicmp(cls, kSkinBoxClass) == 0)
            ensureSkinBoxClass();

        HWND hwnd = ::CreateWindowExW(
            0, cls, text.c_str(),
            // WS_CLIPSIBLINGS 是关键：没有它，兄弟窗互相之间不裁剪，
            // 叠放次序（Level）就算改了，看上去仍是"最后重画的那块在上面"。
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | style,
            x, y, w, h, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
            AfxGetInstanceHandle(), nullptr);
        applyFont(hwnd);
        // 焦点类控件补 WS_TABSTOP：没有它，系统认为这些控件"不可停"，
        // Tab 键既走不过去，自研的 TabIndex 也拿不到候选。
        if (_wcsicmp(cls, L"EDIT") == 0 || _wcsicmp(cls, L"BUTTON") == 0 ||
            _wcsicmp(cls, L"COMBOBOX") == 0 || _wcsicmp(cls, L"LISTBOX") == 0 ||
            _wcsicmp(cls, L"SYSLINK") == 0)
        {
            ::SetWindowLongPtrW(hwnd, GWL_STYLE,
                                ::GetWindowLongPtrW(hwnd, GWL_STYLE) | WS_TABSTOP);
        }
        // owner-draw 控件的外观由 OnDrawItem 全权绘制；若再套 DarkMode_Explorer，
        // 系统会额外描一圈深色边框，白底页上就变成"每个控件都有黑框"
        const bool ownerDraw = ((style & SS_OWNERDRAW) != 0) || ((style & BS_OWNERDRAW) != 0);
        if (!ownerDraw)
            ::SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
        else
        {
            // owner-draw 控件的窗口不该带系统边框：否则客户区比窗口小一圈，
            // 那 1px 系统边框会露出深色，看上去就是"每个控件都有黑框"
            const LONG_PTR st = ::GetWindowLongPtrW(hwnd, GWL_STYLE);
            if ((st & WS_BORDER) != 0)
            {
                ::SetWindowLongPtrW(hwnd, GWL_STYLE, st & ~static_cast<LONG_PTR>(WS_BORDER));
                ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                               SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            // 先取出类过程再换成本进程的绘制过程：类过程负责 BN_CLICKED、
            // 按下/悬停状态与 BM_CLICK，丢掉它按钮就只剩一张画，点不动。
            WNDPROC oldProc = reinterpret_cast<WNDPROC>(
                ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                    reinterpret_cast<LONG_PTR>(&PyMfcOwnerDrawProc)));
            if (oldProc != nullptr)
                ::SetPropW(hwnd, kOldProcProp, reinterpret_cast<HANDLE>(oldProc));
            // owner-draw 控件也要能被光标逻辑找到宿主（按钮/卡片 hover 手型）
            ::SetPropW(hwnd, kSkinHostProp, reinterpret_cast<HANDLE>(this));
        }

        // 选项卡：内容区底色跟随主题，否则深色皮肤里是一块白板
        if (_wcsicmp(cls, L"SysTabControl32") == 0)        {
            ::SetPropW(hwnd, kTabBrushProp, reinterpret_cast<HANDLE>(m_bgBrush));
            WNDPROC tabOld = reinterpret_cast<WNDPROC>(
                ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                    reinterpret_cast<LONG_PTR>(&PyMfcTabBodyProc)));
            if (tabOld != nullptr)
                ::SetPropW(hwnd, kTabOldProcProp, reinterpret_cast<HANDLE>(tabOld));
        }

        // 自绘皮肤接管：控件保留状态与交互，绘制改由控件自己的 WM_PAINT 完成
        if (isSkinPaintClass(cls) || m_scrollAreas.count(static_cast<int>(id)) != 0)
        {
            WNDPROC skinOld = reinterpret_cast<WNDPROC>(
                ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                    reinterpret_cast<LONG_PTR>(&PyMfcSkinWindowProc)));
            if (skinOld != nullptr)
            {
                skinOldProcs()[hwnd] = skinOld;
                ::SetPropW(hwnd, kSkinOldProcProp, reinterpret_cast<HANDLE>(skinOld));
                ::SetPropW(hwnd, kSkinHostProp, reinterpret_cast<HANDLE>(this));
            }
            debugLogf(L"[sknproc] install id=%d cls=%ls old=%p", ::GetDlgCtrlID(hwnd), cls,
                      reinterpret_cast<void*>(skinOld));

            // ListView 的表头是系统自己建的内部子窗口（不经过这里），
            // 不接管它的话表格顶上会留一条原生白条。
            if (_wcsicmp(cls, L"SysListView32") == 0)
            {
                HWND hdr = reinterpret_cast<HWND>(::SendMessageW(hwnd, LVM_GETHEADER, 0, 0));
                if (hdr != nullptr)
                {
                    WNDPROC hdrOld = reinterpret_cast<WNDPROC>(
                        ::SetWindowLongPtrW(hdr, GWLP_WNDPROC,
                                            reinterpret_cast<LONG_PTR>(&PyMfcSkinWindowProc)));
                    if (hdrOld != nullptr)
                    {
                        skinOldProcs()[hdr] = hdrOld;
                        ::SetPropW(hdr, kSkinOldProcProp, reinterpret_cast<HANDLE>(hdrOld));
                        ::SetPropW(hdr, kSkinHostProp, reinterpret_cast<HANDLE>(this));
                    }
                    debugLogf(L"[sknproc] install header id=%d old=%p", ::GetDlgCtrlID(hdr),
                              reinterpret_cast<void*>(hdrOld));
                }
            }
        }

        // 控件级事件：装在最后，链在"当前窗口过程"之后，不影响既有绘制/交互
        if (hwnd != nullptr && m_anyCtlEvent)
        {
            WNDPROC cur = reinterpret_cast<WNDPROC>(
                ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                    reinterpret_cast<LONG_PTR>(&PyMfcEventSubProc)));
            ::SetPropW(hwnd, kEvtPrevProcProp, reinterpret_cast<HANDLE>(cur));
            ::SetPropW(hwnd, kEvtHostProp, reinterpret_cast<HANDLE>(this));
        }

        // 原生控件先按主题默认色上一遍，XAML 声明外观时 setControlAppearance 会再覆盖
        applyNativeThemeColors(hwnd, m_bg, m_text);
        return hwnd;
    }

void PyMfcHost::addButton(int id, const std::wstring& text, int x, int y, int w, int h)
{
        HWND hwnd = createControl(L"BUTTON", text, BS_OWNERDRAW, x, y, w, h, id);
        // =1 表面路径：按钮降级为节点（保留隐藏 HWND 取文字/显隐），宿主状态机承担命中/四态。
        if (m_newSurface)
        {
            registerPaintNode(id, 5, NodeLayerControl, x, y, w, h);
            if (hwnd != nullptr) ::ShowWindow(hwnd, SW_HIDE);
        }
    }

void PyMfcHost::addLabel(int id, const std::wstring& text, int x, int y, int w, int h)
{
        HWND hwnd = createControl(L"STATIC", text, SS_OWNERDRAW, x, y, w, h, id);
        // =1 表面路径：标签无输入，隐藏 HWND + 节点绘制；文字仍由保留句柄承载。
        if (m_newSurface)
        {
            registerPaintNode(id, 4, NodeLayerControl, x, y, w, h);
            if (hwnd != nullptr) ::ShowWindow(hwnd, SW_HIDE);
        }
    }

void PyMfcHost::addEdit(int id, const std::wstring& text, int x, int y, int w, int h)
{
        createControl(L"EDIT", text, WS_BORDER | ES_AUTOHSCROLL, x, y, w, h, id);
    }

void PyMfcHost::addListBox(int id, int x, int y, int w, int h)
{
        createControl(L"LISTBOX", L"", WS_BORDER | LBS_NOTIFY | WS_VSCROLL, x, y, w, h, id);
    }

void PyMfcHost::addCheckBox(int id, const std::wstring& text, int x, int y, int w, int h)
{
        createControl(L"BUTTON", text, BS_AUTOCHECKBOX, x, y, w, h, id);
    }

void PyMfcHost::addRadio(int id, const std::wstring& text, int x, int y, int w, int h)
{
        // 本 host 内所有 radio 视为一组：第一个（尚无 radio 时）加 WS_GROUP 起点
        bool hasRadio = false;
        if (m_hWnd != nullptr)
        {
            for (HWND child = ::GetWindow(m_hWnd, GW_CHILD); child != nullptr;
                 child = ::GetWindow(child, GW_HWNDNEXT))
            {
                if (isClassOf(child, L"BUTTON") &&
                    (::GetWindowLongPtrW(child, GWL_STYLE) & BS_TYPEMASK) == BS_AUTORADIOBUTTON)
                {
                    hasRadio = true;
                    break;
                }
            }
        }
        createControl(L"BUTTON", text,
                      BS_AUTORADIOBUTTON | (hasRadio ? 0 : WS_GROUP), x, y, w, h, id);
    }

void PyMfcHost::addGroup(int id, const std::wstring& text, int x, int y, int w, int h)
{
        createControl(L"BUTTON", text, BS_GROUPBOX, x, y, w, h, id);
    }

void PyMfcHost::addCombo(int id, int x, int y, int w, int h)
{
        // h 要包含展开后的下拉列表高度，否则列表会被裁掉
        createControl(L"COMBOBOX", L"", WS_VSCROLL | CBS_DROPDOWNLIST, x, y, w, h, id);
    }

void PyMfcHost::addEditAdvanced(int id, const std::wstring& text, bool multiline, bool readOnly,
                         int maxLength, const std::wstring& placeholder,
                         int x, int y, int w, int h)
{
        const DWORD style = multiline
                                ? (WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN)
                                : (WS_BORDER | ES_AUTOHSCROLL);
        createControl(L"EDIT", text, style, x, y, w, h, id);
        if (readOnly)
            setReadOnly(id, true);
        if (maxLength > 0)
            setLimitText(id, maxLength);
        if (!placeholder.empty())
            setCueBanner(id, placeholder);
    }

void PyMfcHost::addComboAdvanced(int id, bool dropDownList, int x, int y, int w, int h)
{
        // CBS_DROPDOWN 是"可在下拉项之外手打文字"的可编辑下拉框，CBS_DROPDOWNLIST 只能选
        createControl(L"COMBOBOX", L"",
                      WS_VSCROLL | (dropDownList ? CBS_DROPDOWNLIST : CBS_DROPDOWN),
                      x, y, w, h, id);
    }

void PyMfcHost::addListBoxAdvanced(int id, bool multipleSelect, int x, int y, int w, int h)
{
        createControl(L"LISTBOX", L"", WS_BORDER | LBS_NOTIFY | WS_VSCROLL |
                                            (multipleSelect ? LBS_MULTIPLESEL : 0),
                      x, y, w, h, id);
    }

void PyMfcHost::setLimitText(int id, int n)
{
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::SendMessageW(h, EM_SETLIMITTEXT, static_cast<WPARAM>(n > 0 ? n : 0), 0);
    }

void PyMfcHost::setCueBanner(int id, const std::wstring& text)
{
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::SendMessageW(h, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(text.c_str()));
    }

void PyMfcHost::addProgress(int id, int x, int y, int w, int h)
{
        createControl(L"msctls_progress32", L"", PBS_SMOOTH, x, y, w, h, id);
        setRange(id, 0, 100);
        setPos(id, 0);
    }

void PyMfcHost::addSlider(int id, int minValue, int maxValue, int x, int y, int w, int h)
{
        createControl(L"msctls_trackbar32", L"", TBS_AUTOTICKS | TBS_HORZ, x, y, w, h, id);
        setRange(id, minValue, maxValue);
        setPos(id, minValue);
    }

void PyMfcHost::addTab(int id, int x, int y, int w, int h)
{
        createControl(L"SysTabControl32", L"", WS_TABSTOP, x, y, w, h, id);
    }

void PyMfcHost::addTree(int id, int x, int y, int w, int h)
{
        createControl(L"SysTreeView32", L"",
                      WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | WS_TABSTOP,
                      x, y, w, h, id);
    }

void PyMfcHost::addListCtrl(int id, int x, int y, int w, int h)
{
        // HTML 的 .editor table 没有外框、也没有竖网格线，只有行底 1px 细线
        createControl(L"SysListView32", L"", LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
                      x, y, w, h, id);
        if (HWND hwnd = ::GetDlgItem(m_hWnd, id))
        {
            ::SendMessageW(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                           LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            // 可编辑表格钩子：记录活动单元格 + 拦 F2/Delete/Insert/Ctrl+C/V
            ::SetPropW(hwnd, kGridHostProp, reinterpret_cast<HANDLE>(this));
            {
                WNDPROC gridOld = reinterpret_cast<WNDPROC>(
                    ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                        reinterpret_cast<LONG_PTR>(&PyMfcGridProc)));
                ::SetPropW(hwnd, kGridOldProp, reinterpret_cast<HANDLE>(gridOld));
            }
            // 行高交给字体决定（LVM_SETITEMHEIGHT 传 -1 在本机 SysListView32 上会 AV，别碰）
            // 默认 8pt 太小：换成 13px 的雅黑，行高才跟 HTML 的 12px 表格相当
            static HFONT s_tableFont = nullptr;
            if (s_tableFont == nullptr)
            {
                LOGFONTW lf = {};
                lf.lfHeight = -13;
                lf.lfWeight = FW_NORMAL;
                lf.lfCharSet = DEFAULT_CHARSET;
                lf.lfQuality = CLEARTYPE_QUALITY;
                lf.lfFaceName[0] = L'\0';
                ::wcscpy_s(lf.lfFaceName, L"Microsoft YaHei UI");
                s_tableFont = ::CreateFontIndirectW(&lf);
            }
            if (s_tableFont != nullptr)
                ::SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(s_tableFont), TRUE);
        }
    }

// ------------------------- 新自绘表格 PyMfcTable -------------------------
TableControl* PyMfcHost::findNewTable(int id) const
{
    auto it = m_newTables.find(id);
    return it != m_newTables.end() ? it->second : nullptr;
}

namespace
{
    std::wstring hostClipboardText()
    {
        std::wstring out;
        if (!::OpenClipboard(nullptr)) return out;
        if (HANDLE h = ::GetClipboardData(CF_UNICODETEXT))
        {
            if (const wchar_t* p = static_cast<const wchar_t*>(::GlobalLock(h)))
            {
                out = p;
                ::GlobalUnlock(h);
            }
        }
        ::CloseClipboard();
        return out;
    }
}

struct TableEventCtx
{
    PyMfcHost* host = nullptr;
    int id = 0;
};

void PyMfcHost::tableEventCb(void* ctx, int row, int col, int what)
{
    TableEventCtx* tc = static_cast<TableEventCtx*>(ctx);
    PyMfcHost* h = tc ? tc->host : nullptr;
    if (h == nullptr) return;
    const char* act = "select";
    switch (what)
    {
    case 1: act = "cellclick"; break;
    case 2: act = "dbl";       break;
    case 3: act = "select";    break;
    case 4: act = "edit";       break;
    case 5: act = "delete";     break;
    case 6: act = "insert";     break;
    case 7: act = "paste";      break;
    case 8: act = "sort";       break;   // 点表头 → 按该列排序
    default: break;
    }
    std::wstring text;
    if (act == std::string("paste")) text = hostClipboardText();
    h->fireGridAction(tc ? tc->id : 0, act, row, col, text);
}

void PyMfcHost::addTable(int id, int x, int y, int w, int h)
{
    HWND hw = TableControl::Create(m_hWnd, x, y, w, h, id);
    if (hw == nullptr) return;
    TableControl* tc = TableControl::From(hw);
    if (tc == nullptr) return;
    m_newTables[id] = tc;
    void* evtCtx = m_tableCtx.count(id) ? m_tableCtx[id] : nullptr;
    if (evtCtx == nullptr)
    {
        evtCtx = new TableEventCtx{this, id};
        m_tableCtx[id] = evtCtx;
    }
    tc->SetCallback(&PyMfcHost::tableEventCb, evtCtx);
}

void PyMfcHost::tableClear(int id)
{
    if (TableControl* tc = findNewTable(id)) tc->Clear();
}

int PyMfcHost::tableRowCount(int id) const
{
    if (TableControl* tc = findNewTable(id)) return tc->Rows();
    return 0;
}

void PyMfcHost::tableSetColumns(int id, const std::vector<int>& widths,
                                const std::vector<std::wstring>& titles)
{
    if (TableControl* tc = findNewTable(id)) tc->SetColumns(widths, titles);
}

void PyMfcHost::tableSetColTypes(int id, const std::vector<int>& types)
{
    if (TableControl* tc = findNewTable(id)) tc->SetColTypes(types);
}

void PyMfcHost::tableAppendRow(int id, const std::vector<std::wstring>& cells)
{
    if (TableControl* tc = findNewTable(id)) tc->AppendRow(cells);
}

void PyMfcHost::tableSetCell(int id, int row, int col, const std::wstring& text)
{
    if (TableControl* tc = findNewTable(id)) tc->SetCell(row, col, text);
}

std::wstring PyMfcHost::tableGetCell(int id, int row, int col) const
{
    if (TableControl* tc = findNewTable(id)) return tc->GetText(row, col);
    return L"";
}

py::object PyMfcHost::tableCellRect(int id, int row, int col) const
{
    if (TableControl* tc = findNewTable(id))
    {
        const RECT rc = tc->Cell(row, col);
        return py::make_tuple(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
    }
    return py::none();
}

int PyMfcHost::tableSelectedRow(int id) const
{
    if (TableControl* tc = findNewTable(id)) return tc->Selected();
    return -1;
}

void PyMfcHost::tableSelectRow(int id, int row)
{
    if (TableControl* tc = findNewTable(id)) { tc->Select(row); tc->EnsureVisible(row); }
}

void PyMfcHost::tableEnsureVisible(int id, int row)
{
    if (TableControl* tc = findNewTable(id)) tc->EnsureVisible(row);
}

void PyMfcHost::tableSetFilter(int id, int col, const std::wstring& text)
{
    if (TableControl* tc = findNewTable(id)) tc->SetFilter(col, text);
}

void PyMfcHost::tableClearFilters(int id)
{
    if (TableControl* tc = findNewTable(id)) tc->ClearFilters();
}

void PyMfcHost::tableSort(int id, int col, bool desc)
{
    if (TableControl* tc = findNewTable(id)) tc->SortBy(col, desc);
}

int PyMfcHost::tableViewRows(int id) const
{
    if (TableControl* tc = findNewTable(id)) return tc->ViewRows();
    return 0;
}

void PyMfcHost::addChoice(int id, int x, int y, int w, int h)
{
        createControl(L"COMBOBOX", L"", WS_VSCROLL | CBS_DROPDOWNLIST, x, y, w, h, id);
    }

void PyMfcHost::addDatePicker(int id, int x, int y, int w, int h)
{
        createControl(L"SysDateTimePick32", L"", WS_TABSTOP, x, y, w, h, id);
    }

void PyMfcHost::addPanel(int id, int x, int y, int w, int h)
{
        createControl(L"STATIC", L"", WS_CHILD | WS_VISIBLE, x, y, w, h, id);
    }

void PyMfcHost::addWhiteRect(int id, int x, int y, int w, int h)
{
        HWND hwnd = createControl(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_WHITERECT | WS_BORDER, x, y, w, h, id);
        // =1 表面路径：白板降级为绘制节点，HWND 隐藏（句柄保留，GetDlgItem/SetWindowText 等 API 仍走通）。
        if (m_newSurface)
        {
            registerPaintNode(id, 1, NodeLayerControl, x, y, w, h);
            if (hwnd != nullptr) ::ShowWindow(hwnd, SW_HIDE);
        }
    }

void PyMfcHost::setControlFont(int id, const std::wstring& face, int px, bool bold)
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr || px <= 0) return;
        static std::map<std::wstring, HFONT> cache;
        const std::wstring key = face + L"|" + std::to_wstring(px) + (bold ? L"|b" : L"|n");
        HFONT f = nullptr;
        auto it = cache.find(key);
        if (it != cache.end()) f = it->second;
        else
        {
            LOGFONTW lf = { 0 };
            lf.lfHeight = -px;
            lf.lfWeight = bold ? FW_SEMIBOLD : FW_NORMAL;
            lf.lfCharSet = DEFAULT_CHARSET;
            lf.lfQuality = CLEARTYPE_QUALITY;
            ::wcsncpy_s(lf.lfFaceName, LF_FACESIZE, face.c_str(), _TRUNCATE);
            f = ::CreateFontIndirectW(&lf);
            cache[key] = f;
        }
        if (f != nullptr)
            ::SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(f), TRUE);
    }

void PyMfcHost::setControlAccent(int id, int r, int g, int b)
{
        m_accent[id] = RGB(r, g, b);
        if (HWND h = ::GetDlgItem(m_hWnd, id)) ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::setLabelColor(int id, int r, int g, int b)
{
        m_labelFg[id] = RGB(r, g, b);
        if (HWND h = ::GetDlgItem(m_hWnd, id)) ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::addSplitter(int id, int x, int y, int w, int h)
{
        HWND hwnd = createControl(L"STATIC", L"", WS_CHILD | WS_VISIBLE, x, y, w, h, id);
        // 原生 STATIC 收到鼠标消息什么也不做：不换成自己这个过程，
        // 分隔条就是个画出来的方块，拖不动也影响不到邻居
        if (hwnd != nullptr)
            ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                reinterpret_cast<LONG_PTR>(&PyMfcSplitterProc));
    }

void PyMfcHost::addStaticLine(int id, int x, int y, int w, int h)
{
        // 登记必须早于 createControl：创建过程中就会触发首帧 WM_DRAWITEM，
        // 登记晚一步的话首帧按"普通标签"画出去，之后又不一定再有重绘，
        // 屏幕上就永远停在那张没有线的图（滚动区当年踩的是同一个坑）。
        m_lines.insert(id);
        createControl(L"STATIC", L"", SS_OWNERDRAW, x, y, w, h, id);
        // =1 表面路径：分割线注册为节点；HWND 保留（拖拽热条在 onDrag 后装命中/捕获过程），
        // 其自身 WM_PAINT 由 OwnerDrawProc 在节点存在时短路，不再直绘。
        if (m_newSurface)
            registerPaintNode(id, 3, NodeLayerControl, x, y, w, h);
        debugLogf(L"[line-reg] id=%d hwnd=%p lines=%d", id,
                  static_cast<void*>(::GetDlgItem(m_hWnd, id)), static_cast<int>(m_lines.size()));
    }

void PyMfcHost::addHyperlink(int id, const std::wstring& text, int x, int y, int w, int h)
{
        createControl(L"SysLink", text, WS_TABSTOP, x, y, w, h, id);
    }

void PyMfcHost::addRichText(int id, int x, int y, int w, int h)
{
        createControl(L"RICHEDIT50W", L"", WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL, x, y, w, h, id);
    }

void PyMfcHost::addToggle(int id, const std::wstring& text, int x, int y, int w, int h)
{
        createControl(L"BUTTON", text, WS_TABSTOP | BS_AUTOCHECKBOX, x, y, w, h, id);
    }

void PyMfcHost::addBitmapButton(int id, const std::wstring& text, int x, int y, int w, int h)
{
        createControl(L"BUTTON", text, WS_TABSTOP | BS_OWNERDRAW, x, y, w, h, id);
    }

void PyMfcHost::addStaticBitmap(int id, int x, int y, int w, int h)
{
        createControl(L"STATIC", L"", SS_BITMAP | SS_CENTERIMAGE, x, y, w, h, id);
    }

void PyMfcHost::addGrid(int id, int x, int y, int w, int h)
{
        createControl(L"SysListView32", L"", WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS, x, y, w, h, id);
    }

void PyMfcHost::addScrolledWindow(int id, int x, int y, int w, int h)
{
        // 必须先登记：createControl 里据此决定给这个 Static 装自绘过程
        m_scrollAreas.insert(id);
        createControl(L"STATIC", L"", SS_OWNERDRAW, x, y, w, h, id);
    }

void PyMfcHost::addCalendar(int id, int x, int y, int w, int h)
{
        createControl(L"SysMonthCal32", L"", WS_TABSTOP, x, y, w, h, id);
    }

void PyMfcHost::addSkinBox(int id, int kind, int x, int y, int w, int h)
{
        createControl(kSkinBoxClass, L"", WS_TABSTOP, x, y, w, h, id);
        m_skinKinds[id] = kind;
        if (HWND hwnd = ::GetDlgItem(m_hWnd, id))
        {
            ::SetPropW(hwnd, kSkinKindProp, reinterpret_cast<HANDLE>(static_cast<INT_PTR>(kind)));
            if (kind == kSkinBoxBusy)
            {
                m_busyPhase[id] = 0;
                ::SetTimer(hwnd, kBusyTimerId, 80, nullptr);   // 12fps 旋转
            }
        }
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::addShadowBox(int id, int x, int y, int w, int h, int pad, int radius,
                      int dx, int dy, int blur,
                      const std::wstring& shadowColor, const std::wstring& bgColor,
                      const std::wstring& backColor, const std::wstring& borderColor,
                      int borderW)
{
        pymfcskin::ShadowBoxStyle s;
        s.pad = pad > 0 ? pad : 0;
        s.radius = radius;
        s.dx = dx;
        s.dy = dy;
        s.blur = blur > 0 ? blur : 0;
        s.borderW = borderW;
        int a = 255;
        COLORREF c = 0;
        if (xamlapp::ParseColor(shadowColor, c, &a))
        {
            s.shadow = c;
            s.alpha = a < 0 ? 0 : (a > 255 ? 255 : a);
        }
        COLORREF tmp = 0;
        if (xamlapp::ParseColor(bgColor, tmp)) s.bg = tmp;
        if (xamlapp::ParseColor(backColor, tmp)) s.back = tmp;
        if (xamlapp::ParseColor(borderColor, tmp)) s.border = tmp;

        m_shadowBox[id] = s;
        addSkinBox(id, kSkinBoxShadow, x - s.pad, y - s.pad, w + 2 * s.pad, h + 2 * s.pad);
        // =1 表面路径：阴影底板降级为浮层节点，HWND 隐藏（阴影纯装饰，无输入语义）。
        if (m_newSurface)
        {
            registerPaintNode(id, 2, NodeLayerOverlay, x - s.pad, y - s.pad,
                               w + 2 * s.pad, h + 2 * s.pad);
            if (HWND hw = ::GetDlgItem(m_hWnd, id)) ::ShowWindow(hw, SW_HIDE);
        }
        if (HWND hw = ::GetDlgItem(m_hWnd, id))
        {
            const LONG_PTR ex = ::GetWindowLongPtrW(hw, GWL_EXSTYLE);
            // 默认**不加** WS_EX_TRANSPARENT：加上之后本窗口被合成器当成纯装饰，
            // 宿主重绘时会从父窗口透出黑缝（详情浮层内部出现短黑线）。
            // 阴影区吃不吃鼠标由调用方自己权衡，这里不替它决定。
            (void)ex;
            ::SetWindowLongPtrW(hw, GWL_EXSTYLE, ex & ~WS_EX_TRANSPARENT);
            ::SetWindowPos(hw, nullptr, 0, 0, 0, 0,
                           SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                               SWP_FRAMECHANGED);
        }
    }

// 悬浮光标：真控件手型、画布爪型、容器/空白 nullptr（沿用系统箭头）。
// 只按"这个窗口本身是不是可操作对象"判断，不因为父容器可滚就整片变手型。
HCURSOR PyMfcHost::hoverCursor(HWND hwnd) const
{
        if (hwnd == nullptr)
            return nullptr;
        const int id = ::GetDlgCtrlID(hwnd);

        auto itKind = m_skinKinds.find(id);
        if (itKind != m_skinKinds.end())
        {
            switch (itKind->second)
            {
            case kSkinBoxQCanvas:                 // 画布：静止 grab，按住拖动 grabbing
            case kSkinBoxFlick:
                return pyMfcCanvasCursor(hwnd);
            case kSkinBoxScrollBar:
            case kSkinBoxSwitch:
            case kSkinBoxSpin:
            case kSkinBoxRange:
            case kSkinBoxPages:
            case kSkinBoxDial:
            case kSkinBoxTumbler:
            case kSkinBoxTable:
            case kSkinBoxTree:
            case kSkinBoxListView:
            case kSkinBoxGridView:
            case kSkinBoxPathView:
            case kSkinBoxMenu:
            case kSkinBoxMenuBar:
                return pyMfcHandCursor();
            default:                              // 容器 / 忙碌指示 / 阴影底板：箭头
                return nullptr;
            }
        }

        if (m_scrollAreas.count(id) != 0)
        {
            // 滚动区域只有右侧那一条滚动条是"可点"的，内容区仍是普通箭头
            POINT pt = {};
            ::GetCursorPos(&pt);
            ::ScreenToClient(hwnd, &pt);
            RECT cr = {};
            ::GetClientRect(hwnd, &cr);
            if (pt.x >= cr.right - 18 && pt.y >= cr.top && pt.y < cr.bottom)
                return pyMfcHandCursor();
            return nullptr;
        }

        wchar_t cls[64] = {};
        ::GetClassNameW(hwnd, cls, 64);
        if (_wcsicmp(cls, L"BUTTON") == 0 || _wcsicmp(cls, L"SysLink") == 0 ||
            _wcsicmp(cls, L"ComboBox") == 0)
            return pyMfcHandCursor();

        // 明确绑了点击/双击的控件（卡片、图标、自定义块）同样算可点
        if (!m_click.empty() && m_click.count(static_cast<UINT>(id)) != 0)
            return pyMfcHandCursor();
        if (!m_dblclick.empty() && m_dblclick.count(static_cast<UINT>(id)) != 0)
            return pyMfcHandCursor();
        return nullptr;
}

bool PyMfcHost::skinBoxMouse(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{        const int id = ::GetDlgCtrlID(hwnd);
        auto itKind = m_skinKinds.find(id);
        if (itKind == m_skinKinds.end())
            return false;
        const int kind = itKind->second;
        POINT pt = {static_cast<LONG>(static_cast<short>(LOWORD(lp))),
                    static_cast<LONG>(static_cast<short>(HIWORD(lp)))};
        RECT rc = {};
        ::GetClientRect(hwnd, &rc);

        if (msg == WM_MOUSEWHEEL && (kind == kSkinBoxSpin || kind == kSkinBoxSwitch))
        {
            const int delta = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? 1 : -1;
            if (kind == kSkinBoxSpin)
                stepSpin(id, delta);
            else
                setSwitchValue(id, !getSwitchValue(id));
            ::InvalidateRect(hwnd, nullptr, TRUE);
            return true;
        }

        // 可编辑画布：点选 / 框选 / Delete 删除。默认不开（m_cvEdit 里没有这个 id 就不介入），
        // 这样既有画布用例的点击语义一点没变。
        if (kind == kSkinBoxQCanvas && m_cvEdit.count(id) != 0)
        {
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                ::SetCapture(hwnd);
                m_cvDown[id] = pt;
                const int hit = canvasHitTest(id, pt);
                if (hit >= 0)
                {
                    m_cvBand.erase(id);
                    setCanvasSelection(id, std::vector<int>{hit});
                }
                else
                {
                    m_cvSel[id].clear();
                    m_cvBand[id] = RECT{pt.x, pt.y, pt.x, pt.y};
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            if (msg == WM_MOUSEMOVE && m_cvBand.count(id) != 0)
            {
                POINT d = m_cvDown.count(id) ? m_cvDown[id] : pt;
                RECT b = {d.x, d.y, pt.x, pt.y};
                if (b.right < b.left) { const LONG t = b.left; b.left = b.right; b.right = t; }
                if (b.bottom < b.top) { const LONG t = b.top; b.top = b.bottom; b.bottom = t; }
                m_cvBand[id] = b;
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_LBUTTONUP)
            {
                if (::GetCapture() == hwnd)
                    ::ReleaseCapture();
                auto bi = m_cvBand.find(id);
                if (bi != m_cvBand.end())
                {
                    RECT b = bi->second;
                    m_cvBand.erase(bi);
                    if (b.right - b.left > 4 && b.bottom - b.top > 4)
                    {
                        std::vector<int> picked;
                        auto ii = m_cvItems.find(id);
                        if (ii != m_cvItems.end())
                        {
                            for (int k = 0; k < static_cast<int>(ii->second.size()); ++k)
                            {
                                if (!ii->second[k].selectable)
                                    continue;
                                RECT u = {};
                                if (::IntersectRect(&u, &ii->second[k].box, &b))
                                    picked.push_back(k);
                            }
                        }
                        setCanvasSelection(id, picked);
                    }
                    else
                    {
                        m_cvSel[id].clear();
                        fireCanvasSelect(id);
                        ::InvalidateRect(hwnd, nullptr, TRUE);
                    }
                }
                return true;
            }
            if (msg == WM_KEYDOWN && wp == VK_DELETE)
            {
                removeCanvasSelection(id);
                return true;
            }
        }

        if (kind == kSkinBoxSwitch)
        {
            if (msg == WM_LBUTTONDOWN)
            {
                setSwitchValue(id, !getSwitchValue(id));
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxSpin)
        {
            if (msg == WM_LBUTTONDOWN)
            {
                const int btnW = 18;
                if (pt.x >= rc.right - btnW)
                {
                    const int mid = rc.bottom / 2;
                    stepSpin(id, pt.y < mid ? +1 : -1);
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                    return true;
                }
            }
            if (msg == WM_LBUTTONDBLCLK)
            {
                stepSpin(id, +1);
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxRange)
        {
            const int knob = 14;
            const int lo = rangeMinOf(id);
            const int hi = rangeMaxOf(id);
            const int vlo = getRangeLow(id);
            const int vhi = getRangeHigh(id);
            const int usable = (rc.right - knob) - rc.left;
            auto posOf = [&](int v) -> int {
                const int span = hi > lo ? hi - lo : 0;
                if (span <= 0) return rc.left + knob / 2;
                return rc.left + knob / 2 +
                       static_cast<int>((static_cast<long long>(usable) * (v - lo)) / span);
            };
            auto valueOf = [&](int x) -> int {
                if (usable <= 0 || hi <= lo) return lo;
                int v = lo + static_cast<int>((static_cast<long long>(hi - lo) * (x - rc.left - knob / 2)) / usable);
                return v < lo ? lo : (v > hi ? hi : v);
            };

            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                ::SetCapture(hwnd);
                const int kLo = posOf(vlo);
                const int kHi = posOf(vhi);
                const int dLo = abs(pt.x - kLo);
                const int dHi = abs(pt.x - kHi);
                m_dragThumb[id] = (dLo <= dHi) ? 1 : 2;
                // 直接跳到点击位置：拖动手感更接近 QML 的 Slider
                const int xv = valueOf(pt.x);
                if (m_dragThumb[id] == 1)
                {
                    const int lv = xv < vhi ? xv : vhi;
                    setRangeValues(id, lv, vhi);
                }
                else
                {
                    const int hv = xv > vlo ? xv : vlo;
                    setRangeValues(id, vlo, hv);
                }
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_MOUSEMOVE && m_dragThumb.count(id) != 0)
            {
                const int v = valueOf(pt.x);
                if (m_dragThumb[id] == 1)
                {
                    const int curHi = getRangeHigh(id);
                    setRangeValues(id, v < curHi ? v : curHi, curHi);
                }
                else
                {
                    const int curLo = getRangeLow(id);
                    setRangeValues(id, curLo, v > curLo ? v : curLo);
                }
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_LBUTTONUP)
            {
                m_dragThumb.erase(id);
                ::ReleaseCapture();
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxPages)
        {
            if (msg == WM_LBUTTONDOWN)
            {
                const int count = m_pageCount.count(id) ? m_pageCount[id] : 3;
                const int dot = 8, gap = 6;
                const int totalW = count * dot + (count - 1) * gap;
                const int x0 = (rc.right - totalW) / 2;
                const int idx = (pt.x - x0) / (dot + gap);
                if (idx >= 0 && idx < count)
                {
                    m_pageCur[id] = idx;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                    return true;
                }
            }
            return false;
        }
        if (kind == kSkinBoxDial)
        {
            const int lo = rangeMinOf(id);
            const int hi = rangeMaxOf(id);
            auto valueOf = [&](int px, int py) -> int {
                const int cx = (rc.left + rc.right) / 2;
                const int cy = (rc.top + rc.bottom) / 2;
                double a = std::atan2(static_cast<double>(py - cy), static_cast<double>(px - cx));
                double deg = a * 180.0 / 3.14159265358979323846;   // -180..180
                if (deg < 0.0)
                    deg += 360.0;                                   // 0..360
                // 弧从 135° 起顺时针扫 270°（0..360 里是 135 -> 405，绕回来到 45）
                double t = deg - 135.0;
                if (t < 0.0)
                    t += 360.0;
                if (t > 270.0)
                {
                    // 落在缺口的 90° 里，吸到最近的端点
                    t = t < 315.0 ? 270.0 : 0.0;
                }
                const int span = hi > lo ? hi - lo : 0;
                return lo + static_cast<int>((t * span) / 270.0 + 0.5);
            };
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                ::SetCapture(hwnd);
                m_dragMode[id] = 1;
                m_dialVal[id] = valueOf(pt.x, pt.y);
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_MOUSEMOVE && m_dragMode.count(id) != 0 && m_dragMode[id] == 1)
            {
                m_dialVal[id] = valueOf(pt.x, pt.y);
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_LBUTTONUP && m_dragMode.count(id) != 0)
            {
                m_dragMode.erase(id);
                ::ReleaseCapture();
                return true;
            }
            if (msg == WM_MOUSEWHEEL)
            {
                const int d = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? 1 : -1;
                stepDial(id, d);
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxTumbler)
        {
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                const int h = rc.bottom - rc.top;
                stepTumbler(id, pt.y < rc.top + h / 3 ? 1 : (pt.y > rc.top + (2 * h) / 3 ? -1 : 0));
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_MOUSEWHEEL)
            {
                stepTumbler(id, GET_WHEEL_DELTA_WPARAM(wp) > 0 ? -1 : 1);
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxScrollBar)
        {
            const int lo = rangeMinOf(id);
            const int hi = rangeMaxOf(id);
            const int page = m_scrollPage.count(id) ? m_scrollPage[id] : 10;
            const bool vert = m_scrollVert.count(id) && m_scrollVert[id];
            const int cur = m_scrollVal.count(id) ? m_scrollVal[id] : lo;
            const int span = hi > lo ? hi - lo : 1;
            const int total = vert ? (rc.bottom - rc.top) : (rc.right - rc.left);
            int thumb = static_cast<int>((static_cast<long long>(total) * page) / (span + page));
            if (thumb < 12)
                thumb = 12;
            if (thumb > total)
                thumb = total;
            const int free = total - thumb;
            const int at = vert ? (pt.y - rc.top) : (pt.x - rc.left);

            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                const int t0 = span > 0 ? static_cast<int>((static_cast<long long>(free) * (cur - lo)) / span) : 0;
                if (at >= t0 && at <= t0 + thumb)
                {
                    m_dragMode[id] = 2;
                    m_dragOrigin[id] = at - t0;
                    ::SetCapture(hwnd);
                }
                else
                {
                    // 点轨道：跳一页
                    const int v = cur + (at < t0 ? -page : page);
                    setScrollValueUser(id, v);
                }
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_MOUSEMOVE && m_dragMode.count(id) != 0 && m_dragMode[id] == 2)
            {
                const int pos = at - (m_dragOrigin.count(id) ? m_dragOrigin[id] : 0);
                const int v = free > 0 ? lo + static_cast<int>((static_cast<long long>(span) * pos) / free) : lo;
                setScrollValueUser(id, v);
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_LBUTTONUP && m_dragMode.count(id) != 0)
            {
                m_dragMode.erase(id);
                m_dragOrigin.erase(id);
                ::ReleaseCapture();
                return true;
            }
            if (msg == WM_MOUSEWHEEL)
            {
                const int d = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? -1 : 1;
                setScrollValueUser(id, cur + d * (page / 2 > 1 ? page / 2 : 1));
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxTable)
        {
            const int headH = 26, rowH = 24;
            const int ncol = static_cast<int>(m_tblCols.count(id) ? m_tblCols[id].size() : 0);
            const int total = getTableRowCount(id);
            if (msg == WM_MOUSEWHEEL)
            {
                const int d = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? -1 : 1;
                int first = m_tblScroll.count(id) ? m_tblScroll[id] : 0;
                first += d;
                if (first < 0)
                    first = 0;
                if (first > total - 1)
                    first = total > 0 ? total - 1 : 0;
                m_tblScroll[id] = first;
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                if (pt.y < rc.top + headH)
                {
                    // 点表头 → 排序（同一列再点反向）
                    if (ncol > 0 && cwOf(id) > 0)
                        sortTable(id, (pt.x - rc.left) / cwOf(id));
                    return true;
                }
                const int row = (m_tblScroll.count(id) ? m_tblScroll[id] : 0) +
                                (pt.y - rc.top - headH) / rowH;
                if (row >= 0 && row < total)
                {
                    m_tblSel[id] = row;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxTree)
        {
            const int rowH = 24;
            const std::vector<int> levels = m_treeLevel.count(id) ? m_treeLevel[id] : std::vector<int>();
            // 可见行 -> 原始下标（和绘制时同样的折叠过滤）
            auto rowAt = [&](int visRow) -> int {
                int n = 0;
                for (int i = 0; i < static_cast<int>(m_treeText[id].size()); ++i)
                {
                    if (isTreeRowHidden(id, i))
                        continue;
                    if (n == visRow)
                        return i;
                    ++n;
                }
                return -1;
            };
            if (msg == WM_MOUSEWHEEL)
            {
                const int d = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? -1 : 1;
                int first = m_treeScroll.count(id) ? m_treeScroll[id] : 0;
                first += d;
                if (first < 0)
                    first = 0;
                m_treeScroll[id] = first;
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                const int vis = (m_treeScroll.count(id) ? m_treeScroll[id] : 0) +
                                (pt.y - rc.top) / rowH;
                const int row = rowAt(vis);
                if (row >= 0)
                {
                    // 点在箭头附近 → 展开/折叠；其余位置 → 选中
                    const int level = row < static_cast<int>(levels.size()) ? levels[row] : 0;
                    const int arrowX = rc.left + 6 + level * 16;
                    if (m_treeKids[id][row] && pt.x >= arrowX - 4 && pt.x <= arrowX + 12)
                        toggleTreeRow(id, row);
                    else
                    {
                        m_treeSel[id] = row;
                        ::InvalidateRect(hwnd, nullptr, TRUE);
                    }
                }
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxFlick)
        {
            if (!m_flOff.count(id))
                m_flOff[id] = POINT{0, 0};
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                ::SetCapture(hwnd);
                m_flPos[id] = pt;
                m_flRaw[id] = POINT{m_flOff[id].x, m_flOff[id].y};
                m_flVel[id] = POINT{0, 0};
                m_flStamp[id] = ::GetTickCount64();
                return true;
            }
            if (msg == WM_MOUSEMOVE)
            {
                if (::GetCapture() == hwnd)
                {
                    int vw, vh, cw, ch, minX, minY;
                    flickMetrics(id, vw, vh, cw, ch, minX, minY);
                    POINT raw = m_flRaw.count(id) ? m_flRaw[id] : POINT{0, 0};
                    raw.x += pt.x - m_flPos[id].x;
                    raw.y += pt.y - m_flPos[id].y;
                    m_flRaw[id] = raw;
                    // 越界部分按 0.5 阻尼：拉得动，但拉不远（松手再由回弹收回来）
                    POINT off = raw;
                    if (off.x > 0) off.x = off.x / 2;
                    else if (off.x < minX) off.x = minX + (off.x - minX) / 2;
                    if (off.y > 0) off.y = off.y / 2;
                    else if (off.y < minY) off.y = minY + (off.y - minY) / 2;
                    const POINT last = m_flPos[id];
                    m_flOff[id] = off;
                    flickApply(id);
                    // 速度只取"这一帧的位移"，累积位移会把速度放大成天文数字
                    m_flVel[id] = POINT{(pt.x - last.x) * 60, (pt.y - last.y) * 60};
                    m_flPos[id] = pt;
                    m_flStamp[id] = ::GetTickCount64();
                }
                return true;
            }
            if (msg == WM_LBUTTONUP)
            {
                if (::GetCapture() == hwnd)
                    ::ReleaseCapture();
                const POINT v = m_flVel.count(id) ? m_flVel[id] : POINT{0, 0};
                if (v.x != 0 || v.y != 0)
                    ::SetTimer(hwnd, kFlickTimerId, 16, nullptr);
                return true;
            }
            if (msg == WM_MOUSEWHEEL)
            {
                int vw, vh, cw, ch, minX, minY;
                flickMetrics(id, vw, vh, cw, ch, minX, minY);
                const int d = GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA;
                setFlickOffset(id, m_flOff[id].x, m_flOff[id].y + d * 60);
                (void)minY;
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxPager)
        {
            const int cnt = viewCount(id);
            RECT hr = {};
            ::GetClientRect(hwnd, &hr);
            if (msg == WM_LBUTTONDOWN)
            {
                m_pgDragX[id] = pt.x;
                ::SetFocus(hwnd);
                return true;
            }
            if (msg == WM_LBUTTONUP)
            {
                const int dx = pt.x - (m_pgDragX.count(id) ? m_pgDragX[id] : pt.x);
                if (dx <= -40)
                    viewAnimate(id, viewIndex(id) + 1, 260);
                else if (dx >= 40)
                    viewAnimate(id, viewIndex(id) - 1, 260);
                else if (pt.y >= hr.bottom - 26 && cnt > 0)
                {
                    // 点页码条：左三分之一上一页、右三分之一下一页
                    if (pt.x < hr.left + (hr.right - hr.left) / 3)
                        viewAnimate(id, viewIndex(id) - 1, 200);
                    else if (pt.x > hr.right - (hr.right - hr.left) / 3)
                        viewAnimate(id, viewIndex(id) + 1, 200);
                }
                m_pgDragX[id] = pt.x;
                return true;
            }
            return msg == WM_MOUSEMOVE;
        }

        if (kind == kSkinBoxListView)
        {
            const int rowH = lvRowH(id);
            const int sc = getListScroll(id);
            auto rowAt = [&](int y) -> int {
                const int i = (y + sc) / rowH;
                return i >= 0 && i < getListItemCount(id) ? i : -1;
            };
            if (msg == WM_MOUSEWHEEL)
            {
                const int d = GET_WHEEL_DELTA_WPARAM(wp);
                setListScroll(id, sc - d * rowH * 3 / 120);
                m_flickVel[id] = 0;
                return true;
            }
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                const int i = rowAt(pt.y);
                if (i >= 0)
                    m_lvSel[id] = i;
                m_lvDragY[id] = pt.y;
                m_lvLastY[id] = pt.y;
                m_lvDragScroll[id] = sc;
                m_flickVel[id] = 0;
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_MOUSEMOVE)
            {
                if (wp & MK_LBUTTON)
                {
                    const int dy = m_lvDragY.count(id) ? (m_lvDragY[id] - pt.y) : 0;
                    setListScroll(id, (m_lvDragScroll.count(id) ? m_lvDragScroll[id] : 0) + dy);
                    // 速度采样：只取"这一帧的位移"，累积位移会把速度放大成天文数字。
                    auto lp = m_lvLastY.find(id);
                    const int step = (lp != m_lvLastY.end()) ? (lp->second - pt.y) : 0;
                    m_lvLastY[id] = pt.y;
                    m_flickVel[id] = step * 60;
                    m_flickStamp[id] = ::GetTickCount64();
                    return true;
                }
                const int i = rowAt(pt.y);
                if ((m_lvHover.count(id) ? m_lvHover[id] : -1) != i)
                {
                    m_lvHover[id] = i;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            if (msg == WM_LBUTTONUP)
            {
                if ((m_flickVel.count(id) ? m_flickVel[id] : 0) != 0)
                    ::SetTimer(hwnd, kFlickTimerId, 16, nullptr);   // 松手后惯性
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxGridView)
        {
            RECT cgr;
            ::GetClientRect(hwnd, &cgr);
            int cols = 1, cw = 92, chh = 96;
            gvMetrics(id, cols, cw, chh);
            const int viewW = cgr.right;
            const int viewH = cgr.bottom;
            const int cnt = getListItemCount(id);
            const int rows = (cnt + cols - 1) / cols;
            const int maxSc = (rows * chh - viewH) > 0 ? (rows * chh - viewH) : 0;
            const int sc = getListScroll(id);
            auto cellAt = [&](int x, int y) -> int {
                const int col = x / cw;
                const int row = (y + sc) / chh;
                if (col < 0 || col >= cols || row < 0)
                    return -1;
                const int i = row * cols + col;
                return i < cnt ? i : -1;
            };
            if (msg == WM_MOUSEWHEEL)
            {
                const int d = GET_WHEEL_DELTA_WPARAM(wp);
                setListScroll(id, sc - d * chh * 3 / 120);
                m_flickVel[id] = 0;
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                const int i = cellAt(pt.x, pt.y);
                if (i >= 0)
                    m_lvSel[id] = i;
                m_lvDragY[id] = pt.y;
                m_lvLastY[id] = pt.y;
                m_lvDragScroll[id] = sc;
                m_flickVel[id] = 0;
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_MOUSEMOVE)
            {
                if (wp & MK_LBUTTON)
                {
                    const int dy = m_lvDragY.count(id) ? (m_lvDragY[id] - pt.y) : 0;
                    setListScroll(id, (m_lvDragScroll.count(id) ? m_lvDragScroll[id] : 0) + dy);
                    auto lp = m_lvLastY.find(id);
                    const int step = (lp != m_lvLastY.end()) ? (lp->second - pt.y) : 0;
                    m_lvLastY[id] = pt.y;
                    m_flickVel[id] = step * 60;
                    m_flickStamp[id] = ::GetTickCount64();
                    return true;
                }
                const int i = cellAt(pt.x, pt.y);
                if ((m_lvHover.count(id) ? m_lvHover[id] : -1) != i)
                {
                    m_lvHover[id] = i;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            if (msg == WM_LBUTTONUP)
            {
                if ((m_flickVel.count(id) ? m_flickVel[id] : 0) != 0)
                    ::SetTimer(hwnd, kFlickTimerId, 16, nullptr);
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxPathView)
        {
            RECT cpr;
            ::GetClientRect(hwnd, &cpr);
            const int cnt = getListItemCount(id);
            auto stepTo = [&](int idx) {
                if (cnt <= 0)
                    return;
                if (idx < 0) idx = 0;
                if (idx > cnt - 1) idx = cnt - 1;
                if ((m_lvSel.count(id) ? m_lvSel[id] : -1) != idx)
                {
                    m_lvSel[id] = idx;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
            };
            const int cur = m_lvSel.count(id) ? m_lvSel[id] : 0;
            if (msg == WM_MOUSEWHEEL)
            {
                const int d = GET_WHEEL_DELTA_WPARAM(wp);
                stepTo(cur - d / 120);
                return true;
            }
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                stepTo(cpr.right / 2 > pt.x ? cur - 1 : cur + 1);
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxMenu)
        {
            const int row = menuRowAt(id, pt.y);
            if (msg == WM_MOUSEMOVE)
            {
                if ((m_menuHover.count(id) ? m_menuHover[id] : -1) != row)
                {
                    m_menuHover[id] = row;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            if (msg == WM_LBUTTONDOWN)
            {
                if (row >= 0)
                {
                    m_menuSel[id] = row;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                    ::ShowWindow(hwnd, SW_HIDE);
                }
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxMenuBar)
        {
            const int cellW = 88;
            auto indexAt = [&](int x) -> int {
                const int i = (x - rc.left - 2) / cellW;
                if (x < rc.left + 2 || i >= static_cast<int>(m_mbItems[id].size()))
                    return -1;
                return i;
            };
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                const int i = indexAt(pt.x);
                if (i >= 0)
                {
                    m_mbSel[id] = i;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            if (msg == WM_MOUSEMOVE)
            {
                const int i = indexAt(pt.x);
                if ((m_mbHover.count(id) ? m_mbHover[id] : -1) != i)
                {
                    m_mbHover[id] = i;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxToolTip)
        {
            // ToolTip 不吃点击：让底下的窗口照常收到
            return false;
        }

        if (kind == kSkinBoxPopup)
        {
            if (msg == WM_LBUTTONDOWN)
            {
                ::ShowWindow(hwnd, SW_HIDE);     // 点正文即关闭
                return true;
            }
            return msg == WM_MOUSEMOVE;
        }

        if (kind == kSkinBoxToolBar)
        {
            const int btnW = 74, step = btnW + 4;
            auto indexAt = [&](int x) -> int {
                const int i = (x - rc.left - 4) / step;
                if (x < rc.left + 4 || i >= static_cast<int>(m_tbItems[id].size()))
                    return -1;
                return i;
            };
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                const int i = indexAt(pt.x);
                if (i >= 0)
                {
                    m_tbSel[id] = i;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            if (msg == WM_MOUSEMOVE)
            {
                const int i = indexAt(pt.x);
                if ((m_tbHover.count(id) ? m_tbHover[id] : -1) != i)
                {
                    m_tbHover[id] = i;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            return false;
        }
        return false;
    }

int PyMfcHost::cwOf(int id) const
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return 0;
        RECT rc = {};
        ::GetClientRect(h, &rc);
        const auto it = m_tblCols.find(id);
        const int n = it != m_tblCols.end() ? static_cast<int>(it->second.size()) : 0;
        return n > 0 ? (rc.right - rc.left) / n : 0;
    }

int PyMfcHost::rangeMinOf(int id) const
{
        auto it = m_boxRange.find(id);
        return it != m_boxRange.end() ? it->second.first : 0;
    }

int PyMfcHost::rangeMaxOf(int id) const
{
        auto it = m_boxRange.find(id);
        return it != m_boxRange.end() ? it->second.second : 100;
    }

int PyMfcHost::getRangeLow(int id) const
{
        auto it = m_rangeLo.find(id);
        return it != m_rangeLo.end() ? it->second : rangeMinOf(id);
    }

int PyMfcHost::getRangeHigh(int id) const
{
        auto it = m_rangeHi.find(id);
        return it != m_rangeHi.end() ? it->second : rangeMaxOf(id);
    }

void PyMfcHost::setRangeValues(int id, int low, int high)
{
        m_rangeLo[id] = low;
        m_rangeHi[id] = high;
    }

bool PyMfcHost::getSwitchValue(int id) const
{
        auto it = m_switchOn.find(id);
        return it != m_switchOn.end() && it->second;
    }

void PyMfcHost::setSwitchValue(int id, bool on)
{
        m_switchOn[id] = on;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

int PyMfcHost::getSpinValue(int id) const
{
        auto it = m_spinVal.find(id);
        return it != m_spinVal.end() ? it->second : 0;
    }

void PyMfcHost::setSpinValue(int id, int value)
{
        {
            int d = 0;
            std::string e;
            if (!m_animApplying && behaviorFor(id, L"Value", d, e))
            {
                animateFrom(id, L"Value", getSpinValue(id), value, d, e, 0, false);
                return;
            }
        }
        const int lo = m_spinLo.count(id) ? m_spinLo[id] : 0;
        const int hi = m_spinHi.count(id) ? m_spinHi[id] : 100;
        int v = value < lo ? lo : (value > hi ? hi : value);
        m_spinVal[id] = v;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::setSpinRange(int id, int lo, int hi)
{
        m_spinLo[id] = lo;
        m_spinHi[id] = hi;
        if (m_spinVal.count(id) == 0 || m_spinVal[id] < lo || m_spinVal[id] > hi)
            m_spinVal[id] = lo;
    }

void PyMfcHost::stepSpin(int id, int delta)
{
        setSpinValue(id, getSpinValue(id) + delta);
    }

void PyMfcHost::bumpBusyPhase(int id)
{
        auto it = m_busyPhase.find(id);
        m_busyPhase[id] = (it != m_busyPhase.end() ? it->second : 0) + 1;
    }

void PyMfcHost::setPageCount(int id, int count)
{ m_pageCount[id] = count; }


int PyMfcHost::getPageCount(int id) const
{
        auto it = m_pageCount.find(id);
        return it != m_pageCount.end() ? it->second : 0;
    }

void PyMfcHost::setPageIndex(int id, int index)
{
        const int count = getPageCount(id);
        if (count <= 0)
            return;
        m_pageCur[id] = index < 0 ? 0 : (index >= count ? count - 1 : index);
    }

int PyMfcHost::getPageIndex(int id) const
{
        auto it = m_pageCur.find(id);
        return it != m_pageCur.end() ? it->second : 0;
    }

int PyMfcHost::getDial(int id) const
{
        auto it = m_dialVal.find(id);
        return it != m_dialVal.end() ? it->second : 0;
    }

void PyMfcHost::setDial(int id, int value)
{
        {
            int d = 0;
            std::string e;
            if (!m_animApplying && behaviorFor(id, L"Value", d, e))
            {
                animateFrom(id, L"Value", getDial(id), value, d, e, 0, false);
                return;
            }
        }
        const int lo = rangeMinOf(id);
        const int hi = rangeMaxOf(id);
        int v = value < lo ? lo : (value > hi ? hi : value);
        m_dialVal[id] = v;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::stepDial(int id, int delta)
{
        setDial(id, getDial(id) + delta);
    }

int PyMfcHost::getTumblerCount(int id) const
{
        auto it = m_tumblerCount.find(id);
        return it != m_tumblerCount.end() ? it->second : 10;
    }

void PyMfcHost::setTumblerCount(int id, int count)
{
        const int c = count > 0 ? count : 1;
        m_tumblerCount[id] = c;
        if (auto it = m_tumblerVal.find(id); it != m_tumblerVal.end() && it->second >= c)
            it->second = c - 1;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

int PyMfcHost::getTumblerValue(int id) const
{
        auto it = m_tumblerVal.find(id);
        return it != m_tumblerVal.end() ? it->second : 0;
    }

void PyMfcHost::setTumblerValue(int id, int index)
{
        const int c = getTumblerCount(id);
        int v = index < 0 ? 0 : (index >= c ? c - 1 : index);
        m_tumblerVal[id] = v;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::stepTumbler(int id, int delta)
{
        if (delta == 0)
            return;
        // 循环滚动，像 QML Tumbler 那样可以绕回
        const int c = getTumblerCount(id);
        int v = getTumblerValue(id) + delta;
        v = ((v % c) + c) % c;
        m_tumblerVal[id] = v;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::setScrollBarRange(int id, int lo, int hi, int page, bool vertical)
{
        m_boxRange[id] = std::make_pair(lo, hi);
        m_scrollPage[id] = page > 0 ? page : 1;
        m_scrollVert[id] = vertical;
        if (m_scrollVal.count(id) == 0 || m_scrollVal[id] < lo)
            m_scrollVal[id] = lo;
        if (m_scrollVal[id] > hi)
            m_scrollVal[id] = hi;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

int PyMfcHost::getScrollBarValue(int id) const
{
        auto it = m_scrollVal.find(id);
        return it != m_scrollVal.end() ? it->second : 0;
    }

void PyMfcHost::setScrollBarValue(int id, int value)
{
        setScrollValueInternal(id, value);
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::setScrollValueInternal(int id, int value)
{
        const int lo = rangeMinOf(id);
        const int hi = rangeMaxOf(id);
        m_scrollVal[id] = value < lo ? lo : (value > hi ? hi : value);
    }

void PyMfcHost::setScrollValueUser(int id, int value)
{
        const int before = m_scrollVal.count(id) != 0 ? m_scrollVal[id] : rangeMinOf(id);
        setScrollValueInternal(id, value);
        const int after = m_scrollVal.count(id) != 0 ? m_scrollVal[id] : before;
        if (after == before)
            return;                       // 夹边/同值不发事件，滚动到底不会反复回调
        auto it = m_value.find(id);
        if (it != m_value.end())
            invokePython(it->second);
        // Scroll 与 ValueChanged 同源：滚条动了也算滚动，统一用 "scroll" 一并通知
        if (m_anyCtlEvent)
            fireControlEvent(id, L"scroll");
    }

void PyMfcHost::setColumnWidth(int id, int col, int px)
{
        if (HWND h = ::GetDlgItem(m_hWnd, id))
        {
            wchar_t cls[64] = {};
            ::GetClassNameW(h, cls, 64);
            if (_wcsicmp(cls, L"SysListView32") == 0)
                ::SendMessageW(h, LVM_SETCOLUMNWIDTH, static_cast<WPARAM>(col),
                               MAKELPARAM(static_cast<short>(px), 0));
            ::InvalidateRect(h, nullptr, FALSE);
        }
    }

void PyMfcHost::setColumnWidths(int id, const std::vector<int>& widths)
{
    HWND h = ::GetDlgItem(m_hWnd, id);
    if (h == nullptr)
        return;
    wchar_t cls[64] = {};
    ::GetClassNameW(h, cls, 64);
    if (_wcsicmp(cls, L"SysListView32") == 0)
    {
        ::SendMessageW(h, WM_SETREDRAW, FALSE, 0);
        for (size_t col = 0; col < widths.size(); ++col)
            ::SendMessageW(h, LVM_SETCOLUMNWIDTH, static_cast<WPARAM>(col),
                           MAKELPARAM(static_cast<short>(widths[col]), 0));
        ::SendMessageW(h, WM_SETREDRAW, TRUE, 0);
    }
    ::InvalidateRect(h, nullptr, TRUE);
}

py::object PyMfcHost::tableMetrics(int id)
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr || !::IsWindow(h))
            return py::make_tuple(0, 0);
        const LRESULT sp = ::SendMessageW(h, LVM_GETITEMSPACING, TRUE, 0);
        const int rowH = static_cast<int>((sp >> 16) & 0xFFFF);
        int hdr = 0;
        if (HWND ph = reinterpret_cast<HWND>(::SendMessageW(h, LVM_GETHEADER, 0, 0)))
        {
            RECT rc = { 0, 0, 1, 1 };
            ::GetWindowRect(ph, &rc);
            hdr = rc.bottom - rc.top;
        }
        return py::make_tuple(rowH > 0 ? rowH : 21, hdr > 0 ? hdr : 24);
    }

void PyMfcHost::setTableColumns(int id, const std::wstring& tabJoined)
{
        m_tblCols[id] = splitTabs(tabJoined);
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::addTableRow(int id, const std::wstring& tabJoined)
{
        std::vector<std::wstring> cells = splitTabs(tabJoined);
        m_tblRows[id].push_back(cells);
        if (HWND h = ::GetDlgItem(m_hWnd, id))
        {
            // addListCtrl 建的是原生 SysListView32：数据只在 m_tblRows 里是看不见的，
            // 必须真的往控件插行，否则表格只显示表头。
            if (isClassOf(h, L"SysListView32"))
            {
                const int row = static_cast<int>(m_tblRows[id].size()) - 1;
                LVITEMW it = {};
                it.mask = LVIF_TEXT;
                it.iItem = row;
                it.iSubItem = 0;
                it.pszText = const_cast<LPWSTR>(cells.empty() ? L"" : cells[0].c_str());
                ::SendMessageW(h, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&it));
                for (int c = 1; c < static_cast<int>(cells.size()); ++c)
                {
                    LVITEMW sub = {};
                    sub.iSubItem = c;
                    sub.pszText = const_cast<LPWSTR>(cells[c].c_str());
                    ::SendMessageW(h, LVM_SETITEMTEXTW, static_cast<WPARAM>(row),
                                   reinterpret_cast<LPARAM>(&sub));
                }
            }
            ::InvalidateRect(h, nullptr, TRUE);
        }
    }

void PyMfcHost::setTableRows(int id, const std::wstring& blob)
{
        // 行间 '\n'、格间 '\t'。整表一次灌进去：逐行 addTableRow 时每条都在控件
        // 重绘开启状态下插行（39 行实测 22.5 ms），这里 WM_SETREDRAW(FALSE) 包住
        // 全部插入，只在最后失效一次。
        std::vector<std::vector<std::wstring>> rows;
        std::wstring cur;
        auto flushRow = [&]() {
            rows.push_back(splitTabs(cur));
            cur.clear();
        };
        for (wchar_t ch : blob)
        {
            if (ch == L'\n')
                flushRow();
            else if (ch != L'\r')
                cur.push_back(ch);
        }
        if (!cur.empty())
            flushRow();

        m_tblRows[id] = rows;
        m_tblSel[id] = -1;
        m_tblScroll[id] = 0;
        if (TableControl* nt = findNewTable(id)) { nt->SetRowsBulk(rows); return; }

        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return;
        wchar_t cls[64] = {};
        ::GetClassNameW(h, cls, 64);
        if (_wcsicmp(cls, L"SysListView32") == 0)
        {
            ::SendMessageW(h, WM_SETREDRAW, FALSE, 0);
            ::SendMessageW(h, LVM_DELETEALLITEMS, 0, 0);
            for (size_t r = 0; r < rows.size(); ++r)
            {
                const std::vector<std::wstring>& cells = rows[r];
                LVITEMW it = {};
                it.mask = LVIF_TEXT;
                it.iItem = static_cast<int>(r);
                it.pszText = const_cast<LPWSTR>(cells.empty() ? L"" : cells[0].c_str());
                ::SendMessageW(h, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&it));
                for (size_t c = 1; c < cells.size(); ++c)
                {
                    LVITEMW sub = {};
                    sub.iSubItem = static_cast<int>(c);
                    sub.pszText = const_cast<LPWSTR>(cells[c].c_str());
                    ::SendMessageW(h, LVM_SETITEMTEXTW, static_cast<WPARAM>(r),
                                   reinterpret_cast<LPARAM>(&sub));
                }
            }
            ::SendMessageW(h, WM_SETREDRAW, TRUE, 0);
        }
        ::InvalidateRect(h, nullptr, TRUE);
}

void PyMfcHost::clearTableRows(int id)
{
        if (TableControl* nt = findNewTable(id)) { nt->Clear(); return; }
        m_tblRows[id].clear();
        m_tblSel[id] = -1;
        m_tblScroll[id] = 0;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
        {
            wchar_t cls[64] = {};
            ::GetClassNameW(h, cls, 64);
            if (_wcsicmp(cls, L"SysListView32") == 0)
                ::SendMessageW(h, LVM_DELETEALLITEMS, 0, 0);
            ::InvalidateRect(h, nullptr, TRUE);
        }
    }

int PyMfcHost::getTableRowCount(int id) const
{
        if (findNewTable(id)) return tableRowCount(id);
        auto it = m_tblRows.find(id);
        return it != m_tblRows.end() ? static_cast<int>(it->second.size()) : 0;
    }

bool PyMfcHost::isTreeItemCollapsed(int id, int row) const
{
        auto it = m_treeCollapsed.find(id);
        if (it == m_treeCollapsed.end() || row < 0 || row >= static_cast<int>(it->second.size()))
            return false;
        return it->second[row];
    }

void PyMfcHost::setTreeItemCollapsed(int id, int row, bool collapsed)
{
        auto it = m_treeCollapsed.find(id);
        if (it == m_treeCollapsed.end() || row < 0 || row >= static_cast<int>(it->second.size()))
            return;
        it->second[row] = collapsed;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

std::wstring PyMfcHost::getTableCell(int id, int row, int col) const
{
        if (TableControl* nt = findNewTable(id)) return nt->GetText(row, col);
        auto it = m_tblRows.find(id);
        if (it == m_tblRows.end() || row < 0 || row >= static_cast<int>(it->second.size()))
            return {};
        const auto& line = it->second[row];
        if (col < 0 || col >= static_cast<int>(line.size()))
            return {};
        return line[col];
    }

void PyMfcHost::setTableCell(int id, int row, int col, const std::wstring& text)
{
        if (TableControl* nt = findNewTable(id)) { nt->SetCell(row, col, text); return; }
        auto it = m_tblRows.find(id);
        if (it == m_tblRows.end() || row < 0 || row >= static_cast<int>(it->second.size()))
            return;
        auto& line = it->second[row];
        if (col < 0 || col >= static_cast<int>(line.size()))
            return;
        line[col] = text;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

int PyMfcHost::getTableSelectedRow(int id) const
{
        if (TableControl* nt = findNewTable(id)) return nt->Selected();
        // 原生 ListView：真实选中项以控件为准（用户点行时宿主状态不会自己更新）
        if (HWND h = ::GetDlgItem(m_hWnd, id))
        {
            wchar_t cls[64] = {};
            ::GetClassNameW(h, cls, 64);
            if (_wcsicmp(cls, L"SysListView32") == 0)
            {
                const int r = static_cast<int>(
                    ::SendMessageW(h, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), LVNI_SELECTED));
                if (r >= 0)
                    return r;
            }
        }
        auto it = m_tblSel.find(id);
        return it != m_tblSel.end() ? it->second : -1;
    }

void PyMfcHost::setTableSelectedRow(int id, int row)
{
        if (TableControl* nt = findNewTable(id)) { nt->Select(row); return; }
        auto it = m_tblRows.find(id);
        const int total = it != m_tblRows.end() ? static_cast<int>(it->second.size()) : 0;
        int v = row < 0 ? -1 : (row >= total ? total - 1 : row);
        const int prevSel = (m_tblSel.find(id) != m_tblSel.end()) ? m_tblSel[id] : -1;
        m_tblSel[id] = v;
        ensureVisible(id, v);
        if (HWND h = ::GetDlgItem(m_hWnd, id))
        {
            wchar_t cls[64] = {};
            ::GetClassNameW(h, cls, 64);
            if (_wcsicmp(cls, L"SysListView32") == 0)
            {
                // 只动两行：清掉旧选中、置新选中。
                // 原先是 for 循环逐项 SetItemState，39 行就触发 39 次重画，
                // 屏幕上能看到蓝色一行行扫过去 —— 这就是"选中时拖影严重"。
                if (prevSel >= 0 && prevSel < total && prevSel != v)
                {
                    LVITEMW st = {};
                    st.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                    st.state = 0;
                    ::SendMessageW(h, LVM_SETITEMSTATE, static_cast<WPARAM>(prevSel),
                                   reinterpret_cast<LPARAM>(&st));
                }
                if (v >= 0 && v < total)
                {
                    LVITEMW st = {};
                    st.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                    st.state = LVIS_SELECTED | LVIS_FOCUSED;
                    ::SendMessageW(h, LVM_SETITEMSTATE, static_cast<WPARAM>(v),
                                   reinterpret_cast<LPARAM>(&st));
                    ::SendMessageW(h, LVM_ENSUREVISIBLE, static_cast<WPARAM>(v), FALSE);
                }
            }
            // erase 参数必须 FALSE：TRUE 会先把整块表格擦白再画，一帧一次闪
            ::InvalidateRect(h, nullptr, FALSE);
        }
    }

void PyMfcHost::sortTable(int id, int col)
{
        auto cols = m_tblCols.find(id);
        auto rows = m_tblRows.find(id);
        if (cols == m_tblCols.end() || rows == m_tblRows.end())
            return;
        if (col < 0 || col >= static_cast<int>(cols->second.size()))
            return;
        bool asc = true;
        auto s = m_tblSortCol.find(id);
        if (s != m_tblSortCol.end() && s->second == col)
            asc = !(m_tblSortAsc.count(id) && m_tblSortAsc[id]);
        m_tblSortCol[id] = col;
        m_tblSortAsc[id] = asc;

        auto keyOf = [&](const std::vector<std::wstring>& line) -> std::wstring {
            return col < static_cast<int>(line.size()) ? line[col] : std::wstring();
        };
        std::stable_sort(rows->second.begin(), rows->second.end(),
                         [&](const std::vector<std::wstring>& a, const std::vector<std::wstring>& b) {
                             // 数字列按数值排，其余按字典序
                             double na = 0, nb = 0;
                             bool oka = parseNum(keyOf(a), na);
                             bool okb = parseNum(keyOf(b), nb);
                             if (oka && okb)
                                 return asc ? na < nb : na > nb;
                             return asc ? keyOf(a) < keyOf(b) : keyOf(a) > keyOf(b);
                         });
        m_tblSel[id] = -1;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::addTreeItem(int id, const std::wstring& text, int level, bool hasKids)
{
        m_treeText[id].push_back(text);
        m_treeLevel[id].push_back(level < 0 ? 0 : level);
        m_treeKids[id].push_back(hasKids);
        m_treeCollapsed[id].push_back(false);
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::clearTreeItems(int id)
{
        m_treeText[id].clear();
        m_treeLevel[id].clear();
        m_treeKids[id].clear();
        m_treeCollapsed[id].clear();
        m_treeSel[id] = -1;
        m_treeScroll[id] = 0;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

int PyMfcHost::getTreeSelectedRow(int id) const
{
        auto it = m_treeSel.find(id);
        return it != m_treeSel.end() ? it->second : -1;
    }

void PyMfcHost::setTreeSelectedRow(int id, int row)
{
        m_treeSel[id] = row;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

bool PyMfcHost::isTreeRowHidden(int id, int row) const
{
        auto lv = m_treeLevel.find(id);
        auto col = m_treeCollapsed.find(id);
        if (lv == m_treeLevel.end() || col == m_treeCollapsed.end())
            return false;
        const std::vector<int>& levels = lv->second;
        const std::vector<bool>& collapsed = col->second;
        // 往上找最近的祖先：只要任一层折叠且本行层级更深，就隐藏
        const int myLevel = row < static_cast<int>(levels.size()) ? levels[row] : 0;
        for (int i = row - 1; i >= 0; --i)
        {
            if (levels[i] < myLevel)
            {
                if (i < static_cast<int>(collapsed.size()) && collapsed[i])
                    return true;
                if (levels[i] == myLevel - 1)
                    break;
            }
        }
        return false;
    }

void PyMfcHost::toggleTreeRow(int id, int row)
{
        auto col = m_treeCollapsed.find(id);
        if (col == m_treeCollapsed.end() || row < 0 || row >= static_cast<int>(col->second.size()))
            return;
        col->second[row] = !col->second[row];
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::addToolBarItem(int id, const std::wstring& text)
{
        m_tbItems[id].push_back(text);
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::clearToolBarItems(int id)
{
        m_tbItems[id].clear();
        m_tbSel[id] = -1;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

int PyMfcHost::getToolBarSelected(int id) const
{
        auto it = m_tbSel.find(id);
        return it != m_tbSel.end() ? it->second : -1;
    }

void PyMfcHost::setToolBarSelected(int id, int index)
{
        m_tbSel[id] = index;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

std::vector<std::wstring> PyMfcHost::splitTabs(const std::wstring& s)
{
        std::vector<std::wstring> out;
        std::wstring cur;
        for (wchar_t ch : s)
        {
            if (ch == L'\t' || ch == L'|')
            {
                out.push_back(cur);
                cur.clear();
            }
            else
            {
                cur.push_back(ch);
            }
        }
        out.push_back(cur);
        return out;
    }

bool PyMfcHost::parseNum(const std::wstring& s, double& out)
{
        if (s.empty())
            return false;
        wchar_t end = 0;
        out = wcstod(s.c_str(), nullptr);
        (void)end;
        return out != 0.0 || s == L"0";
    }

void PyMfcHost::ensureVisible(int id, int row)
{
        if (row < 0)
            return;
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return;
        RECT rc = {};
        ::GetClientRect(h, &rc);
        auto kind = m_skinKinds.find(id);
        const int headH = (kind != m_skinKinds.end() && kind->second == kSkinBoxTable) ? 26 : 0;
        const int visible = (rc.bottom - rc.top - headH) / 24;
        int first = m_tblScroll.count(id) ? m_tblScroll[id] : 0;
        if (kind != m_skinKinds.end() && kind->second == kSkinBoxTree)
            first = m_treeScroll.count(id) ? m_treeScroll[id] : 0;
        if (row < first)
            first = row;
        else if (row >= first + visible)
            first = row - visible + 1;
        if (first < 0)
            first = 0;
        if (kind != m_skinKinds.end() && kind->second == kSkinBoxTree)
            m_treeScroll[id] = first;
        else
            m_tblScroll[id] = first;
    }

void PyMfcHost::addTabItem(int id, const std::wstring& text)
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr || !isClassOf(h, L"SysTabControl32"))
            return;
        TCITEMW item = {};
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<LPWSTR>(text.c_str());
        const int index = TabCtrl_GetItemCount(h);
        ::SendMessageW(h, TCM_INSERTITEMW, static_cast<WPARAM>(index),
                       reinterpret_cast<LPARAM>(&item));
        // 只选中页码、不真的切换可见页，避免页签看起来"没反应"
        if (index == 0)
            ::SendMessageW(h, TCM_SETCURFOCUS, 0, 0);
        ::InvalidateRect(h, nullptr, TRUE);
        ::UpdateWindow(h);
    }

void PyMfcHost::addTreeNode(int id, const std::wstring& text, int parentIndex)
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr || !isClassOf(h, L"SysTreeView32"))
            return;
        HTREEITEM parent = TVI_ROOT;
        if (parentIndex >= 0)
        {
            auto it = m_treeNodes.find(id);
            if (it != m_treeNodes.end() && parentIndex < static_cast<int>(it->second.size()))
                parent = it->second[static_cast<size_t>(parentIndex)];
            else
                return;  // 父节点不存在时宁可不插，也不插成根节点骗人
        }
        TVINSERTSTRUCTW ins = {};
        ins.hParent = parent;
        ins.hInsertAfter = TVI_LAST;
        ins.item.mask = TVIF_TEXT;
        ins.item.pszText = const_cast<LPWSTR>(text.c_str());
        const HTREEITEM node = reinterpret_cast<HTREEITEM>(
            ::SendMessageW(h, TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&ins)));
        if (node != nullptr)
            m_treeNodes[id].push_back(node);
        // 父节点必须先展开，否则子节点插在收起的分支里，界面上只看得到根 —— 又变成"空壳"
        if (parent == TVI_ROOT)
            ::SendMessageW(h, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(node));
        else
            ::SendMessageW(h, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(parent));
        ::InvalidateRect(h, nullptr, TRUE);
        ::UpdateWindow(h);
    }

void PyMfcHost::addColumn(int id, const std::wstring& text, int width)
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr || !isClassOf(h, L"SysListView32"))
            return;
        HWND header = reinterpret_cast<HWND>(::SendMessageW(h, LVM_GETHEADER, 0, 0));
        const int index = header != nullptr
            ? static_cast<int>(::SendMessageW(header, HDM_GETITEMCOUNT, 0, 0))
            : 0;
        LVCOLUMNW col = {};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        col.cx = width > 0 ? width : 120;
        col.pszText = const_cast<LPWSTR>(text.c_str());
        col.iSubItem = index;
        ::SendMessageW(h, LVM_INSERTCOLUMNW, static_cast<WPARAM>(index),
                       reinterpret_cast<LPARAM>(&col));
        ::InvalidateRect(h, nullptr, TRUE);
        ::UpdateWindow(h);
    }

void PyMfcHost::addRow(int id, const std::wstring& text)
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr || !isClassOf(h, L"SysListView32"))
            return;
        // 一个字符串里用 \t 分隔多列：表格控件要能一次填满一行，而不是只有第一列
        const int row = ListView_GetItemCount(h);
        const std::wstring firstCell = text.substr(0, text.find(L'\t'));
        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = row;
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(firstCell.c_str());
        ::SendMessageW(h, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));

        size_t start = text.find(L'\t');
        int sub = 1;
        while (start != std::wstring::npos)
        {
            const size_t next = text.find(L'\t', start + 1);
            const std::wstring cell = (next == std::wstring::npos)
                ? text.substr(start + 1)
                : text.substr(start + 1, next - start - 1);
            LVITEMW si = {};
            si.mask = LVIF_TEXT;
            si.iItem = row;
            si.iSubItem = sub;
            si.pszText = const_cast<LPWSTR>(cell.c_str());
            ::SendMessageW(h, LVM_SETITEMTEXTW, static_cast<WPARAM>(row),
                           reinterpret_cast<LPARAM>(&si));
            ++sub;
            start = next;
        }
        ::InvalidateRect(h, nullptr, TRUE);
        ::UpdateWindow(h);
    }

void PyMfcHost::setText(int id, const std::wstring& text)
{
        // 超链接的窗口文本必须是链接标记，直接 SetWindowText 会把标记冲掉，
        // 所以显示文字单独记着，由 applyLinkText 组回 <a href="..."> 形式
        if (m_urls.count(id) != 0)
        {
            m_links[id] = text;
            applyLinkText(id);
            return;
        }
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::SetWindowTextW(h, text.c_str());
    }

void PyMfcHost::applyLinkText(int id)
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        auto u = m_urls.find(id);
        if (h == nullptr || u == m_urls.end())
            return;
        std::wstring label = m_links.count(id) != 0 ? m_links[id] : std::wstring();
        if (label.empty() || label.find(L"<") != std::wstring::npos)
            label = u->second;
        const std::wstring markup =
            L"<a href=\"" + u->second + L"\">" + label + L"</a>";
        ::SetWindowTextW(h, markup.c_str());
    }

std::wstring PyMfcHost::getText(int id) const
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (!h)
            return {};
        const int len = ::GetWindowTextLengthW(h);
        std::wstring text(static_cast<size_t>(len) + 1, L'\0');
        ::GetWindowTextW(h, text.data(), len + 1);
        text.resize(static_cast<size_t>(len));
        return text;
    }

void PyMfcHost::setVisible(int id, bool visible)
{
    if (auto it = m_nodes.find(id); it != m_nodes.end())
    {
        it->second.visible = visible;
        it->second.dirty = true;
    }
    if (HWND h = ::GetDlgItem(m_hWnd, id))
        ::ShowWindow(h, visible ? SW_SHOW : SW_HIDE);
}

void PyMfcHost::setEnabled(int id, bool enabled)
{
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::EnableWindow(h, enabled ? TRUE : FALSE);
    }

void PyMfcHost::moveControl(int id, int x, int y, int w, int h)
{
    updatePaintNodeRect(id, x, y, w, h, true);
    if (HWND hwnd = ::GetDlgItem(m_hWnd, id))
        ::MoveWindow(hwnd, x, y, w, h, TRUE);
}

void PyMfcHost::moveMany(py::list items)
{
        const int n = static_cast<int>(items.size());
        if (n <= 0 || m_hWnd == nullptr)
            return;
        HDWP dwp = ::BeginDeferWindowPos(n);
        if (dwp == nullptr)
            return;
        std::vector<HWND> moved;
        moved.reserve(static_cast<size_t>(n));
        std::vector<RECT> oldRects;              // 每个控件挪动前的父客户区矩形
        oldRects.reserve(static_cast<size_t>(n));
        for (py::handle h : items)
        {
            py::sequence s = py::reinterpret_borrow<py::sequence>(h);
            if (s.size() < 6)
                continue;
            const int id = py::cast<int>(s[0]);
            HWND c = ::GetDlgItem(m_hWnd, id);
            if (c == nullptr)
                continue;
            const int x = py::cast<int>(s[1]);
            const int y = py::cast<int>(s[2]);
            const int w = py::cast<int>(s[3]);
            const int hh = py::cast<int>(s[4]);
            const bool vis = py::cast<bool>(s[5]);
            updatePaintNodeRect(id, x, y, w, hh, vis);
            UINT flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW;
            flags |= vis ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;
            RECT before = { 0, 0, 0, 0 };
            ::GetWindowRect(c, &before);
            ::MapWindowPoints(HWND_DESKTOP, m_hWnd, reinterpret_cast<LPPOINT>(&before), 2);
            const bool curVis = ::IsWindowVisible(c) != 0;
            const bool sameRect = before.left == x && before.top == y &&
                                  before.right == x + w && before.bottom == y + hh;
            if (sameRect && curVis == vis)
                continue;  // 位置与显隐都没变：不进 DeferWindowPos，也不重复失效
            dwp = ::DeferWindowPos(dwp, c, nullptr, x, y, w, hh, flags);
            if (dwp == nullptr)
                return;
            moved.push_back(c);
            oldRects.push_back(before);
        }
        ::EndDeferWindowPos(dwp);
        // 只把"动过的子窗口"和父窗口露出的空白标脏。不用 RDW_ALLCHILDREN，
        // 否则会把 D2D 画布也拉进来整张重画，滚动一下子就卡。
        // erase=FALSE：控件都是整块自绘（画布自己 fillRect 底色、原生控件自己画底），
        // 带 TRUE 会先擦成一窗白再画，肉眼就是"闪一下"。
        for (HWND c : moved)
        {
            ::InvalidateRect(c, nullptr, FALSE);
            // 关键：控件挪走后，旧位置是**父容器**的地盘。父容器自己不被标脏，
            // 主窗又被 WS_CLIPCHILDREN 裁掉这一块 —— 旧内容就留在屏幕上，即"拖影"。
            if (HWND p = ::GetParent(c); p != nullptr && p != m_hWnd)
                ::InvalidateRect(p, nullptr, FALSE);
        }
        // 父窗口只标"动过的并集矩形"，不再整窗 RDW_INVALIDATE：
        // 整窗标脏会把工具栏、状态栏、小地图一起拉来重画，而它们根本没动。
        RECT uni = { 0, 0, 0, 0 };
        bool hasUni = false;
        for (size_t i = 0; i < moved.size(); ++i)
        {
            RECT now = { 0, 0, 0, 0 };
            ::GetWindowRect(moved[i], &now);
            ::MapWindowPoints(HWND_DESKTOP, m_hWnd, reinterpret_cast<LPPOINT>(&now), 2);
            RECT both = oldRects[i];
            ::UnionRect(&both, &oldRects[i], &now);
            if (!hasUni)
            {
                uni = both;
                hasUni = true;
            }
            else
            {
                ::UnionRect(&uni, &uni, &both);
            }
        }
        if (hasUni)
            ::InvalidateRect(m_hWnd, &uni, FALSE);
    }

void PyMfcHost::setSurface(int id, int x, int y, int w, int h, int fill)
{
        SurfaceItem& it = m_surfaces[id];
        it.rc = RECT{ x, y, x + w, y + h };
        it.fill = static_cast<COLORREF>(fill & 0x00FFFFFF);
        it.visible = true;
        if (std::find(m_surfaceOrder.begin(), m_surfaceOrder.end(), id) == m_surfaceOrder.end())
            m_surfaceOrder.push_back(id);
        if (m_hWnd != nullptr)
            ::InvalidateRect(m_hWnd, &it.rc, FALSE);
    }

void PyMfcHost::moveSurfaces(py::list items)
{
        for (py::handle h : items)
        {
            py::sequence sq = py::reinterpret_borrow<py::sequence>(h);
            if (sq.size() < 6)
                continue;
            const int id = py::cast<int>(sq[0]);
            auto it = m_surfaces.find(id);
            if (it == m_surfaces.end())
                continue;
            RECT before = it->second.rc;
            const int sx = py::cast<int>(sq[1]);
            const int sy = py::cast<int>(sq[2]);
            it->second.rc = RECT{ sx, sy, sx + py::cast<int>(sq[3]), sy + py::cast<int>(sq[4]) };
            it->second.visible = py::cast<bool>(sq[5]);
            if (m_hWnd != nullptr)
            {
                RECT both = before;
                ::UnionRect(&both, &before, &it->second.rc);
                ::InvalidateRect(m_hWnd, &both, FALSE);
            }
        }
    }

py::tuple PyMfcHost::getSurfaceRect(int id)
{
        auto it = m_surfaces.find(id);
        if (it == m_surfaces.end())
            return py::make_tuple(-1, -1, -1, -1);
        const RECT& rc = it->second.rc;
        return py::make_tuple(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
    }

// ---------------------------------------------------------------------------
// S2-A 绘制节点：注册 / 几何同步 / 一帧 z 序绘制。
// =0 旧 GDI 路径下这些表只登记不参与出帧；=1 表面 OnPaint 走 drawPaintNodes。
// ---------------------------------------------------------------------------
void PyMfcHost::registerPaintNode(int id, int kind, int layer, int x, int y, int w, int h)
{
    XamlPaintNode& n = m_nodes[id];
    n.id = id;
    n.rc = RECT{ x, y, x + w, y + h };
    n.layer = layer;
    n.visible = true;
    n.dirty = true;
    n.kind = kind;
    n.order = static_cast<int>(m_nodeOrder.size());
    if (std::find(m_nodeOrder.begin(), m_nodeOrder.end(), id) == m_nodeOrder.end())
        m_nodeOrder.push_back(id);

    n.hWnd = ::GetDlgItem(m_hWnd, id);

    const int nid = id;
    switch (kind)
    {
    case 1:  // whiteRect
        n.draw = [this, nid](XamlCanvas& cv, const RECT&) {
            auto it = m_nodes.find(nid);
            if (it != m_nodes.end()) drawNodeWhiteRect(cv, it->second);
        };
        break;
    case 2:  // shadowBox
        n.draw = [this, nid](XamlCanvas& cv, const RECT&) {
            auto it = m_nodes.find(nid);
            if (it != m_nodes.end()) drawNodeShadowBox(cv, it->second);
        };
        break;
    case 3:  // staticLine
        n.draw = [this, nid](XamlCanvas& cv, const RECT&) {
            auto it = m_nodes.find(nid);
            if (it != m_nodes.end()) drawNodeStaticLine(cv, it->second);
        };
        break;
    case 4:  // label
        n.draw = [this, nid](XamlCanvas& cv, const RECT&) {
            auto it = m_nodes.find(nid);
            if (it != m_nodes.end()) drawNodeLabel(cv, it->second);
        };
        break;
    case 5:  // button
        n.draw = [this, nid](XamlCanvas& cv, const RECT&) {
            auto it = m_nodes.find(nid);
            if (it != m_nodes.end()) drawNodeButton(cv, it->second);
        };
        break;
    default:
        break;
    }
}

void PyMfcHost::updatePaintNodeRect(int id, int x, int y, int w, int h, bool visible)
{
    auto it = m_nodes.find(id);
    if (it == m_nodes.end()) return;
    it->second.rc = RECT{ x, y, x + w, y + h };
    it->second.visible = visible;
    it->second.dirty = true;
}

const PyMfcHost::XamlPaintNode* PyMfcHost::findPaintNode(int id) const
{
    auto it = m_nodes.find(id);
    return it != m_nodes.end() ? &it->second : nullptr;
}

void PyMfcHost::drawNodeWhiteRect(XamlCanvas& cv, const XamlPaintNode& n) const
{
    COLORREF face = RGB(255, 255, 255);
    const xamlapp::XamlAppearance* ap = findAppearance(n.id);
    if (ap != nullptr && !ap->backColor.empty())
    {
        COLORREF c = 0;
        if (xamlapp::ParseColor(ap->backColor, c))
            face = xamlapp::Blend(c, m_bg, ap->opacity);
    }
    cv.fillRect(n.rc, face);
    // 与旧 SS_WHITERECT | WS_BORDER 对齐：1px 主题边框。
    cv.strokeRect(n.rc, m_border, 1);
}

void PyMfcHost::drawNodeShadowBox(XamlCanvas& cv, const XamlPaintNode& n) const
{
    auto it = m_shadowBox.find(n.id);
    if (it == m_shadowBox.end()) return;
    pymfcskin::drawShadowBox(cv, n.rc, it->second);
}

void PyMfcHost::drawNodeStaticLine(XamlCanvas& cv, const XamlPaintNode& n) const
{
    const RECT& itemRc = n.rc;
    const int wSpan = static_cast<int>(itemRc.right - itemRc.left);
    const int hSpan = static_cast<int>(itemRc.bottom - itemRc.top);
    cv.fillRect(itemRc, surfaceColorAt(CRect(itemRc)));
    if (hSpan > wSpan * 3)
    {
        if (wSpan >= 3)
        {
            cv.fillRect(itemRc, RGB(0xE4, 0xE7, 0xEC));
            const int x = (itemRc.left + itemRc.right) / 2;
            cv.fillRect(RECT{ x, itemRc.top, x + 1, itemRc.bottom }, m_border);
        }
        else
        {
            const int x = (itemRc.left + itemRc.right) / 2;
            cv.fillRect(RECT{ x, itemRc.top, x + 1, itemRc.bottom }, m_border);
        }
    }
    else
    {
        const int y = (itemRc.top + itemRc.bottom) / 2;
        cv.fillRect(RECT{ itemRc.left, y, itemRc.right, y + 1 }, m_border);
    }
}

void PyMfcHost::drawOwnerNode(XamlCanvas& cv, const XamlPaintNode& n, bool isStatic) const
{
    const int cid = n.id;
    const HWND hw = n.hWnd;
    const RECT& itemRc = n.rc;

    CString text;
    if (hw != nullptr)
    {
        ::GetWindowTextW(hw, text.GetBufferSetLength(512), 512);
        text.ReleaseBuffer();
    }

    const xamlapp::XamlAppearance* ap = findAppearance(cid);
    const bool styled = ap != nullptr && !ap->empty();
    const LONG_PTR wst = hw != nullptr ? ::GetWindowLongPtrW(hw, GWL_STYLE) : 0;
    const bool pressed = n.pressed;
    const bool disabled = false;

    const COLORREF surface = surfaceColorAt(CRect(itemRc));
    auto accentIt = m_accent.find(cid);
    const bool hasAccent = (accentIt != m_accent.end()) && !isStatic;
    const COLORREF accent = hasAccent ? accentIt->second : m_border;

    if (styled)
    {
        xamlDrawBackplate(cv, itemRc, *ap, surface, m_border);
    }
    else
    {
        cv.fillRect(itemRc, surface);
        const COLORREF face = isStatic ? surface
                                       : (hasAccent ? RGB(255, 255, 255)
                                                    : (pressed ? RGB(80, 90, 104) : m_btn));
        cv.fillRoundRect(itemRc, face, isStatic ? surface : (hasAccent ? accent : m_border), 8);
    }

    COLORREF back = surface;
    if (styled && !ap->backColor.empty())
    {
        COLORREF c = 0;
        if (xamlapp::ParseColor(ap->backColor, c))
            back = xamlapp::Blend(c, surface, ap->opacity);
    }
    COLORREF fg = disabled ? RGB(140, 140, 140) : contrastText(back);
    if (styled && !ap->foreColor.empty())
    {
        COLORREF c = 0;
        if (xamlapp::ParseColor(ap->foreColor, c))
            fg = disabled ? RGB(140, 140, 140) : c;
    }
    if (hasAccent)
        fg = disabled ? RGB(140, 140, 140) : accent;
    auto labelIt = m_labelFg.find(cid);
    if (labelIt != m_labelFg.end() && !disabled)
        fg = labelIt->second;

    UINT align = isStatic ? DT_LEFT : DT_CENTER;
    if (styled)
        align = xamlapp::TextFlag(*ap);

    RECT trc = itemRc;
    if (styled && align == DT_LEFT)
        trc.left += 6;

    if (const std::wstring* iconPath = findIcon(cid))
    {
        const int side = (itemRc.bottom - itemRc.top) - 8;
        if (side > 0)
        {
            RECT ir = { itemRc.left + 4, itemRc.top + 4, itemRc.left + 4 + side, itemRc.top + 4 + side };
            if (text.IsEmpty())
            {
                ir.left = itemRc.left + ((itemRc.right - itemRc.left) - side) / 2;
                ir.right = ir.left + side;
            }
            if (ir.right <= itemRc.right)
            {
                cv.drawImage(ir, *iconPath);
                if (!text.IsEmpty())
                {
                    align = DT_LEFT;
                    const int textLeft = ir.right + 4;
                    if (trc.left < textLeft)
                        trc.left = textLeft;
                }
            }
        }
    }

    const UINT bsType = static_cast<UINT>(wst) & 0x0F;
    const bool isRadio = (bsType == BS_RADIOBUTTON) || (bsType == BS_AUTORADIOBUTTON);
    const bool isCheckButton = (bsType == BS_CHECKBOX || bsType == BS_AUTOCHECKBOX ||
                                bsType == BS_3STATE || bsType == BS_AUTO3STATE || isRadio);
    if (isCheckButton && hw != nullptr)
    {
        const int boxSide = 16;
        const int h = itemRc.bottom - itemRc.top;
        if (h >= boxSide + 8)
        {
            const int top = itemRc.top + (h - boxSide) / 2;
            RECT box = { itemRc.left + 4, top, itemRc.left + 4 + boxSide, top + boxSide };
            cv.fillRoundRect(box, back, m_border, 3);
            const LRESULT chk = ::SendMessageW(hw, BM_GETCHECK, 0, 0);
            if (chk == BST_CHECKED)
            {
                RECT inner = { box.left + 3, box.top + 3, box.right - 3, box.bottom - 3 };
                if (isRadio)
                {
                    inner.left += 3;
                    inner.top += 3;
                    inner.right -= 3;
                    inner.bottom -= 3;
                }
                cv.fillRoundRect(inner, RGB(0, 120, 212), RGB(0, 120, 212), isRadio ? 7 : 2);
            }
            align = DT_LEFT;
            trc.left = box.right + 6;
            if (trc.left >= trc.right)
                trc.left = itemRc.left + 4;
        }
    }

    const int ptSize = (styled && ap->fontSize > 0) ? ap->fontSize : 0;
    const std::wstring family = (styled && !ap->fontFamily.empty()) ? ap->fontFamily : std::wstring();
    const std::wstring sText(text.GetString(), static_cast<size_t>(text.GetLength()));
    cv.text(trc, sText, fg, align, ptSize, family);

    if (n.focus && !styled)
        cv.strokeRoundRect(itemRc, RGB(0, 120, 212), 8, 1);
}

void PyMfcHost::drawNodeLabel(XamlCanvas& cv, const XamlPaintNode& n) const
{
    drawOwnerNode(cv, n, true);
}

void PyMfcHost::drawNodeButton(XamlCanvas& cv, const XamlPaintNode& n) const
{
    drawOwnerNode(cv, n, false);
}

int PyMfcHost::hitNode(const CPoint& pt) const
{
    for (int id : m_nodeOrder)
    {
        auto it = m_nodes.find(id);
        if (it == m_nodes.end() || !it->second.visible) continue;
        const int k = it->second.kind;
        if (k != 4 && k != 5) continue;
        if (PtInRect(&it->second.rc, pt))
            return id;
    }
    return -1;
}

void PyMfcHost::OnMouseMove(UINT /*nFlags*/, CPoint pt)
{
    const int hit = hitNode(pt);
    for (int id : m_nodeOrder)
    {
        auto it = m_nodes.find(id);
        if (it == m_nodes.end() || it->second.kind != 5) continue;
        const bool want = (id == hit);
        if (want != it->second.hover)
        {
            it->second.hover = want;
            ::InvalidateRect(m_hWnd, &it->second.rc, FALSE);
        }
    }
}

void PyMfcHost::OnLButtonDown(UINT /*nFlags*/, CPoint pt)
{
    const int hit = hitNode(pt);
    if (hit < 0) return;
    auto it = m_nodes.find(hit);
    if (it == m_nodes.end()) return;
    it->second.pressed = true;
    ::SetCapture(m_hWnd);
    ::InvalidateRect(m_hWnd, &it->second.rc, FALSE);
}

void PyMfcHost::OnLButtonUp(UINT /*nFlags*/, CPoint pt)
{
    const int hit = hitNode(pt);
    if (hit < 0) return;
    auto it = m_nodes.find(hit);
    if (it == m_nodes.end()) return;
    const bool wasPressed = it->second.pressed;
    it->second.pressed = false;
    ::ReleaseCapture();
    ::InvalidateRect(m_hWnd, &it->second.rc, FALSE);
    if (wasPressed)
    {
        // 与 BN_CLICKED 同一入口：直接走宿主命令分发，Python m_click 回调同参同时序。
        OnCommandRange(static_cast<UINT>(hit));
        it->second.focus = true;
    }
}

void PyMfcHost::drawPaintNodes(XamlCanvas& cv)
{
    std::vector<const XamlPaintNode*> ordered;
    ordered.reserve(m_nodes.size());
    for (int id : m_nodeOrder)
    {
        auto it = m_nodes.find(id);
        if (it != m_nodes.end() && it->second.visible)
            ordered.push_back(&it->second);
    }
    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const XamlPaintNode* a, const XamlPaintNode* b) {
                         if (a->layer != b->layer) return a->layer < b->layer;
                         return a->order < b->order;
                     });

    // 底板层：m_surfaces 装饰矩形（旧路径在 OnEraseBkgnd 铺底，表面路径由节点帧承担）。
    for (int sid : m_surfaceOrder)
    {
        auto sit = m_surfaces.find(sid);
        if (sit == m_surfaces.end() || !sit->second.visible) continue;
        cv.fillRect(sit->second.rc, sit->second.fill);
    }

    for (const XamlPaintNode* n : ordered)
    {
        if (n->draw == nullptr) continue;
        cv.clipPush(n->rc);
        n->draw(cv, n->rc);
        cv.clipPop();
    }
}

void PyMfcHost::attachXaml(std::shared_ptr<XamlRuntime> rt, const std::map<std::wstring, int>& ids)
{
        m_xaml = std::move(rt);
        m_xamlIds = ids;
    }

bool PyMfcHost::reflowXaml()
{
        if (!m_xaml || m_xaml->root() == nullptr)
            return false;
        CRect rc;
        ::GetClientRect(m_hWnd, &rc);
        m_xaml->applyExpressions();
        m_xaml->layout(0, 0, rc.Width(), rc.Height());

        int moved = 0, texts = 0;
        std::function<void(XamlElement&)> walk = [&](XamlElement& el) {
            auto hit = m_xamlIds.find(el.name);
            if (!el.name.empty() && hit != m_xamlIds.end())
            {
                moveControl(hit->second, el.previewX, el.previewY, el.previewW, el.previewH);
                ++moved;
                // 只有"算出来的文字"才回写控件：普通 Text 是用户输入初值，不能覆盖
                const std::wstring* rawText = nullptr;
                for (const auto& kv : el.attrs)
                    if (_wcsicmp(kv.first.c_str(), L"Text") == 0)
                        rawText = &kv.second;
                if (rawText != nullptr && xamlIsExpression(*rawText))
                {
                    if (HWND h = ::GetDlgItem(m_hWnd, hit->second))
                    {
                        ::SetWindowTextW(h, el.get(L"Text").c_str());
                        ++texts;
                    }
                }
            }
            for (auto& c : el.children)
                walk(c);
        };
        walk(*m_xaml->root());
        ::InvalidateRect(m_hWnd, nullptr, TRUE);
        (void)texts;
        return moved > 0;
    }

bool PyMfcHost::setXamlProperty(const std::wstring& name, const std::wstring& prop, const std::wstring& value)
{
        if (!m_xaml)
            return false;
        if (!m_xaml->setProperty(name, prop, value))
            return false;
        return reflowXaml();
    }

void PyMfcHost::ensureOverlay(int id, int kind)
{
        if (::GetDlgItem(m_hWnd, id) == nullptr)
            addSkinBox(id, kind, 0, 0, 10, 10);
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::ShowWindow(h, SW_HIDE);
    }

void PyMfcHost::hideOverlay(int id)
{
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::ShowWindow(h, SW_HIDE);
    }

bool PyMfcHost::isOverlayVisible(int id) const
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        return h != nullptr && ::IsWindowVisible(h) != FALSE;
    }

void PyMfcHost::placeOverlay(int id, int x, int y, int w, int hgt)
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return;
        RECT crc = {};
        ::GetClientRect(m_hWnd, &crc);
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x + w > crc.right) x = crc.right - w;
        if (y + hgt > crc.bottom) y = crc.bottom - hgt;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        ::SetWindowPos(h, HWND_TOP, x, y, w, hgt, SWP_SHOWWINDOW | SWP_NOACTIVATE);
        ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::showToolTip(int ownerId, const std::wstring& text, int durationMs)
{
        const int tipId = 9001;
        ensureOverlay(tipId, kSkinBoxToolTip);
        m_tipText[tipId] = text;
        int w = 28;
        for (wchar_t ch : text)
            w += (ch < 128 ? 8 : 14);
        if (w > 320) w = 320;
        const int hgt = 28;
        HWND owner = ::GetDlgItem(m_hWnd, ownerId);
        int x = 8, y = 8;
        if (owner != nullptr)
        {
            RECT r = {};
            ::GetWindowRect(owner, &r);
            POINT p = {r.right, r.top};
            ::MapWindowPoints(HWND_DESKTOP, m_hWnd, &p, 1);
            x = p.x + 4;
            y = p.y;
        }
        placeOverlay(tipId, x, y, w, hgt);
        if (HWND h = ::GetDlgItem(m_hWnd, tipId))
        {
            ::KillTimer(h, kTipTimerId);
            if (durationMs > 0)
                ::SetTimer(h, kTipTimerId, static_cast<UINT>(durationMs), nullptr);
        }
    }

void PyMfcHost::hideToolTip()
{
        hideOverlay(9001);
    }

void PyMfcHost::showMenu(int id, const std::vector<std::wstring>& items, int x, int y)
{
        ensureOverlay(id, kSkinBoxMenu);
        m_menuItems[id] = items;
        m_menuHover[id] = -1;
        m_menuSel[id] = -1;
        int w = 120;
        for (const auto& s : items)
        {
            int cw = 24;
            for (wchar_t ch : s)
                cw += (ch < 128 ? 8 : 14);
            if (cw > w) w = cw;
        }
        int hgt = 8;
        for (const auto& s : items)
            hgt += (s == L"-") ? 8 : 26;
        placeOverlay(id, x, y, w, hgt + 4);
    }

void PyMfcHost::hideMenu(int id)
{
        hideOverlay(id);
    }

int PyMfcHost::menuRowAt(int id, int y) const
{
        auto it = m_menuItems.find(id);
        if (it == m_menuItems.end())
            return -1;
        int cy = 4;
        for (int i = 0; i < static_cast<int>(it->second.size()); ++i)
        {
            const int rowH = (it->second[i] == L"-") ? 8 : 26;
            if (y >= cy && y < cy + rowH)
                return it->second[i] == L"-" ? -1 : i;
            cy += rowH;
        }
        return -1;
    }

int PyMfcHost::getMenuSelected(int id) const
{
        auto it = m_menuSel.find(id);
        return it != m_menuSel.end() ? it->second : -1;
    }

void PyMfcHost::setMenuItems(int id, const std::vector<std::wstring>& items)
{
        m_menuItems[id] = items;
        m_menuHover[id] = -1;
        m_menuSel[id] = -1;
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::addMenuBarItem(int id, const std::wstring& text)
{
        m_mbItems[id].push_back(text);
    }

void PyMfcHost::clearMenuBarItems(int id)
{
        m_mbItems[id].clear();
        m_mbSel[id] = -1;
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
    }

int PyMfcHost::getMenuBarSelected(int id) const
{
        auto it = m_mbSel.find(id);
        return it != m_mbSel.end() ? it->second : -1;
    }

void PyMfcHost::setMenuBarSelected(int id, int i)
{
        m_mbSel[id] = i;
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::showPopup(int id, const std::wstring& title, const std::vector<std::wstring>& lines,
                   int x, int y, int w, int hgt)
{
        ensureOverlay(id, kSkinBoxPopup);
        m_popTitle[id] = title;
        m_popLines[id] = lines;
        placeOverlay(id, x, y, w, hgt);
    }

void PyMfcHost::hidePopup(int id)
{
        hideOverlay(id);
    }

bool PyMfcHost::isPopupVisible(int id) const
{
        return isOverlayVisible(id);
    }

void PyMfcHost::gvMetrics(int id, int& cols, int& cw, int& ch) const
{
        RECT rc = {};
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h != nullptr)
            ::GetClientRect(h, &rc);
        const int viewW = (rc.right - rc.left) > 8 ? (rc.right - rc.left) : 92;
        auto iw = m_gvCellW.find(id);
        const int wantW = (iw != m_gvCellW.end() && iw->second > 24) ? iw->second : 92;
        cols = viewW / wantW;
        if (cols < 1) cols = 1;
        cw = (iw != m_gvCellW.end() && iw->second > 24) ? wantW : (viewW / cols);
        auto ih = m_gvCellH.find(id);
        ch = (ih != m_gvCellH.end() && ih->second > 24) ? ih->second : (cw * 96 / 92);
        if (ch < 24) ch = 24;
    }

int PyMfcHost::lvRowH(int id) const
{
        auto it = m_lvItemH.find(id);
        if (it != m_lvItemH.end() && it->second > 8)
            return it->second;
        const auto k = m_skinKinds.find(id);
        if (k != m_skinKinds.end() && k->second == kSkinBoxGridView)
        {
            int cols = 1, cw = 92, ch = 96;
            gvMetrics(id, cols, cw, ch);
            return ch;
        }
        return 44;
    }

int PyMfcHost::lvMaxScroll(int id) const
{
        auto it = m_lvT.find(id);
        const int cnt = it != m_lvT.end() ? static_cast<int>(it->second.size()) : 0;
        RECT rc = {};
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h != nullptr)
            ::GetClientRect(h, &rc);
        const int viewH = rc.bottom - rc.top;
        int rows = cnt;
        const auto k = m_skinKinds.find(id);
        if (k != m_skinKinds.end() && k->second == kSkinBoxGridView)
        {
            int cols = 1, cw = 92, ch = 96;
            gvMetrics(id, cols, cw, ch);
            rows = (cnt + cols - 1) / cols;
            const int need2 = rows * ch - viewH;
            return need2 > 0 ? need2 : 0;
        }
        const int need = rows * lvRowH(id) - viewH;
        return need > 0 ? need : 0;
    }

void PyMfcHost::lvClampScroll(int id)
{
        const int max = lvMaxScroll(id);
        int v = m_lvScroll.count(id) ? m_lvScroll[id] : 0;
        if (v > max) v = max;
        if (v < 0) v = 0;
        m_lvScroll[id] = v;
    }

void PyMfcHost::fitPlanCanvas(int id)
{
        PlanCanvasState& st = m_planCv[id];
        float w = 0.0f, h = 0.0f;
        if (st.mode == 1) { w = static_cast<float>(st.plan.width); h = static_cast<float>(st.plan.height); }
        else if (st.mode == 2) { w = st.svg.width; h = st.svg.height; }
        RECT rc = {0, 0, 1, 1};
        if (HWND h2 = ::GetDlgItem(m_hWnd, id))
            ::GetClientRect(h2, &rc);
        if (w <= 0.0f || h <= 0.0f || rc.right <= 2 || rc.bottom <= 2)
        {
            debugLogf(L"[plan-fit] id=%d skip client=%dx%d plan=%gx%g",
                      id, rc.right, rc.bottom, w, h);
            return;
        }
        const float kw = (static_cast<float>(rc.right) - 8.0f) / w;
        const float kh = (static_cast<float>(rc.bottom) - 8.0f) / h;
        const float raw = (std::min)(kw, kh);
        if (m_newSurface)
        {
            // 新骨架：不封顶 1.0；地板 0.05，上限 4.0，偏移允许负/居中。
            st.zoom = clampPlanZoom(raw);
            st.ox = static_cast<int>((rc.right - w * st.zoom) * 0.5f);
            st.oy = static_cast<int>((rc.bottom - h * st.zoom) * 0.5f);
        }
        else
        {
            // =0 旧行为：HTML clampK [0.2, 4]，fit 封顶 1.0，偏移不允许为负。
            st.zoom = (std::min)((std::min)(kw, kh), 1.0f);
            if (st.zoom < 0.2f) st.zoom = 0.2f;
            st.ox = static_cast<int>((rc.right - w * st.zoom) * 0.5f);
            st.oy = static_cast<int>((rc.bottom - h * st.zoom) * 0.5f);
            if (st.ox < 0) st.ox = 0;
            if (st.oy < 0) st.oy = 0;
        }
        debugLogf(L"[plan-fit] id=%d client=%dx%d plan=%gx%g rawZoom=%.4f fitZoom=%.4f ox=%d oy=%d",
                  id, rc.right, rc.bottom, w, h, raw, st.zoom, st.ox, st.oy);
    }

void PyMfcHost::addPowerCanvas(int id, int x, int y, int w, int h)
{
        // 登记早于创建：创建过程里就有首帧 WM_PAINT，晚登记会按"普通标签"画一次
        m_planCv[id];
        HWND hwnd = createControl(L"STATIC", L"", SS_OWNERDRAW | WS_HSCROLL, x, y, w, h, id);
        if (hwnd != nullptr)
        {
            ::SetPropW(hwnd, kPlanHostProp, reinterpret_cast<HANDLE>(this));
            // XAML 里 Data 属性可能先于建窗落到状态里（那时客户区还是 1×1，算不出 fit）：
            // 窗口一建好就重算一次视图，否则首帧停在 1:1 左上角。
            PlanCanvasState& st = m_planCv[id];
            if (st.mode != 0)
                fitPlanCanvas(id);
            debugLogf(L"[plan-cv] id=%d hwnd=%p mode=%d zoom=%.2f", id,
                      static_cast<void*>(hwnd), st.mode, st.zoom);
        }
    }

void PyMfcHost::setPowerCanvasPlan(int id, const std::wstring& json)
{
        PlanCanvasState& st = m_planCv[id];
        std::vector<xamlplan::PowerNode> nodes;
        std::wstring title, err;
        bool hz = false;
        if (!xamlplan::parseJson(json, nodes, title, err, &hz))
        {
            debugLogf(L"[plan-cv] id=%d json 解析失败: %ls", id, err.c_str());
            return;
        }
        xamlplan::PowerPlanStyle style;
        if (!xamlplan::buildPlan(nodes, st.plan, style, err, hz))
        {
            debugLogf(L"[plan-cv] id=%d 布局失败: %ls", id, err.c_str());
            return;
        }
        st.mode = 1;
        st.sel.clear();
        fitPlanCanvas(id);
        debugLogf(L"[plan-cv] id=%d plan nodes=%d boxes=%d links=%d canvas=%dx%d dir=%ls zoom=%.2f",
                  id, static_cast<int>(st.plan.nodes.size()),
                  static_cast<int>(st.plan.boxes.size()),
                  static_cast<int>(st.plan.links.size()),
                  st.plan.width, st.plan.height, st.plan.dir.c_str(), st.zoom);
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::setPowerCanvasScene(int id, const std::wstring& json)
{
        // 坐标由外部（Python/JSON 引擎）算好；这里只解析 + 建视图，不跑布局算法
        PlanCanvasState& st = m_planCv[id];
        xamlplan::PowerPlan plan;
        std::wstring err;
        if (!xamlplan::parseScene(json, plan, err))
        {
            debugLogf(L"[plan-cv] id=%d 场景解析失败: %ls", id, err.c_str());
            return;
        }
        st.plan = std::move(plan);
        st.mode = 1;
        st.sel.clear();
        fitPlanCanvas(id);
        debugLogf(L"[plan-cv] id=%d scene nodes=%d boxes=%d links=%d canvas=%dx%d dir=%ls zoom=%.2f",
                  id, static_cast<int>(st.plan.nodes.size()),
                  static_cast<int>(st.plan.boxes.size()),
                  static_cast<int>(st.plan.links.size()),
                  st.plan.width, st.plan.height, st.plan.dir.c_str(), st.zoom);
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::copyPowerCanvasScene(int toId, int fromId)
{
    auto it = m_planCv.find(fromId);
    if (it == m_planCv.end())
        return;
    PlanCanvasState& st = m_planCv[toId];
    st.plan = it->second.plan;      // 结构体整体拷贝（字符串/向量都是深拷贝）
    st.mode = 1;
    st.sel.clear();
    fitPlanCanvas(toId);
    debugLogf(L"[plan-cv] id=%d <- copy of %d (nodes=%d links=%d)",
              toId, fromId, static_cast<int>(st.plan.nodes.size()),
              static_cast<int>(st.plan.links.size()));
}

void PyMfcHost::setPowerCanvasSvg(int id, const std::wstring& svgText)
{
        // 转 UTF-8 后交给解析器（内部按 UTF-8 处理实体与文本）
        std::wstring src = svgText;
        std::string utf8;
        if (!src.empty())
        {
            const int n = ::WideCharToMultiByte(CP_UTF8, 0, src.c_str(),
                                                static_cast<int>(src.size()), nullptr, 0, nullptr, nullptr);
            if (n > 0)
            {
                utf8.resize(static_cast<size_t>(n));
                ::WideCharToMultiByte(CP_UTF8, 0, src.c_str(), static_cast<int>(src.size()),
                                      utf8.data(), n, nullptr, nullptr);
            }
        }
        PlanCanvasState& st = m_planCv[id];
        std::wstring err;
        if (!xamlsvg::parseSvgText(utf8, st.svg, err))
        {
            debugLogf(L"[plan-cv] id=%d svg 解析失败: %ls", id, err.c_str());
            return;
        }
        st.mode = 2;
        fitPlanCanvas(id);
        debugLogf(L"[plan-cv] id=%d svg shapes=%d texts=%d size=%.0fx%.0f zoom=%.2f",
                  id, static_cast<int>(st.svg.shapes.size()),
                  static_cast<int>(st.svg.texts.size()),
                  st.svg.width, st.svg.height, st.zoom);
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::setPowerCanvasView(int id, float zoom, int offsetX, int offsetY)
{
        PlanCanvasState& st = m_planCv[id];
        if (m_newSurface)
            st.zoom = clampPlanZoom(zoom);
        else
            st.zoom = (zoom < 0.2f) ? 0.2f : (zoom > 4.0f ? 4.0f : zoom);
        st.ox = offsetX;
        st.oy = offsetY;
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, FALSE);
    }

float PyMfcHost::getPowerCanvasZoom(int id) const
{
        auto it = m_planCv.find(id);
        return it != m_planCv.end() ? it->second.zoom : 0.0f;
    }

std::wstring PyMfcHost::getPowerCanvasSelection(int id) const
{
        auto it = m_planCv.find(id);
        return it != m_planCv.end() ? it->second.sel : std::wstring();
    }

void PyMfcHost::setPowerCanvasOptions(int id, bool showCable, bool mini, int miniOf)
{
        PlanCanvasState& st = m_planCv[id];
        st.showCable = showCable;
        st.mini = mini;
        st.miniOf = miniOf;
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
        if (miniOf >= 0)
            ::InvalidateRect(::GetDlgItem(m_hWnd, miniOf), nullptr, TRUE);
    }

void PyMfcHost::getPowerCanvasView(int id, float& zoom, int& ox, int& oy) const
{
        zoom = 0.0f;
        ox = oy = 0;
        auto it = m_planCv.find(id);
        if (it == m_planCv.end()) return;
        zoom = it->second.zoom;
        ox = it->second.ox;
        oy = it->second.oy;
    }

void PyMfcHost::setDetailCard(int id, py::list rect, py::list texts)
{
    auto it = m_planCv.find(id);
    if (it == m_planCv.end()) return;
    PlanCanvasState& st = it->second;
    if (rect.size() >= 4)
    {
        const int x = py::cast<int>(rect[0]);
        const int y = py::cast<int>(rect[1]);
        const int w = py::cast<int>(rect[2]);
        const int h = py::cast<int>(rect[3]);
        st.detailRc = RECT{ x, y, x + w, y + h };
    }
    st.detailVisible = true;
    st.detailTexts.clear();
    for (py::handle h : texts)
    {
        py::sequence sq = py::reinterpret_borrow<py::sequence>(h);
        if (sq.size() < 8) continue;
        DetailText dt;
        const int x = py::cast<int>(sq[0]);
        const int y = py::cast<int>(sq[1]);
        const int w = py::cast<int>(sq[2]);
        const int h = py::cast<int>(sq[3]);
        dt.rc = RECT{ x, y, x + w, y + h };
        dt.text = py::cast<std::wstring>(sq[4]);
        dt.size = py::cast<int>(sq[5]);
        dt.bold = py::cast<bool>(sq[6]);
        py::sequence rgb = py::reinterpret_borrow<py::sequence>(sq[7]);
        const int r = py::cast<int>(rgb[0]);
        const int g = py::cast<int>(rgb[1]);
        const int b = py::cast<int>(rgb[2]);
        dt.color = RGB(r, g, b);
        st.detailTexts.push_back(std::move(dt));
    }
    if (HWND hw = ::GetDlgItem(m_hWnd, id))
        ::InvalidateRect(hw, nullptr, FALSE);
}

void PyMfcHost::clearDetailCard(int id)
{
    auto it = m_planCv.find(id);
    if (it == m_planCv.end()) return;
    PlanCanvasState& st = it->second;
    st.detailVisible = false;
    st.detailRc = RECT{ 0, 0, 0, 0 };
    st.detailTexts.clear();
    if (HWND hw = ::GetDlgItem(m_hWnd, id))
        ::InvalidateRect(hw, nullptr, FALSE);
}

bool PyMfcHost::detailHit(int id, int clientX, int clientY) const
{
    auto it = m_planCv.find(id);
    if (it == m_planCv.end() || !it->second.detailVisible) return false;
    const RECT& rc = it->second.detailRc;
    return clientX >= rc.left && clientX < rc.right && clientY >= rc.top && clientY < rc.bottom;
}

std::wstring PyMfcHost::planHit(int id, int clientX, int clientY)
{
        auto it = m_planCv.find(id);
        if (it == m_planCv.end() || it->second.mode != 1)
            return {};
        PlanCanvasState& st = it->second;
        if (detailHit(id, clientX, clientY))
            return {};
        const float mx = (static_cast<float>(clientX) - st.ox) / st.zoom;
        const float my = (static_cast<float>(clientY) - st.oy) / st.zoom;
        debugLogf(L"[plan-hit] id=%d client=%d,%d zoom=%.4f off=%d,%d model=%.1f,%.1f boxes=%d",
                  id, clientX, clientY, st.zoom, st.ox, st.oy, mx, my,
                  static_cast<int>(st.plan.boxes.size()));
        for (const xamlplan::PowerBox& b : st.plan.boxes)
        {
            if (mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h)
                return b.code;
        }
        // 连线命中：贝塞尔折线到光标的最近距离（容差按屏幕 8px 折算成模型单位）。
        // 点中一条线 = 选中它的下游节点：卡片描边 + 父链边高亮 + 详情一起对上。
        const float tol = 8.0f / (st.zoom > 0.01f ? st.zoom : 0.01f);
        for (const xamlplan::PowerLink& l : st.plan.links)
        {
            // 与绘制路径（PyMfcCommon.h 折线）保持一致：4 个拐点直接连成正交折线，
            // 不再做贝塞尔采样；命中距离按折线各线段点到线段计算。
            std::vector<xamlgeom::Point2> pts;
            for (int q = 0; q < 4; ++q)
            {
                xamlgeom::Point2 p(l.bez[q * 2], l.bez[q * 2 + 1]);
                if (pts.empty() || pts.back().x != p.x || pts.back().y != p.y)
                    pts.push_back(p);
            }
            float best = 3.4e38f;
            for (size_t i = 1; i < pts.size(); ++i)
            {
                const float ax = static_cast<float>(pts[i - 1].x);
                const float ay = static_cast<float>(pts[i - 1].y);
                const float bx = static_cast<float>(pts[i].x);
                const float by = static_cast<float>(pts[i].y);
                const float vx = bx - ax, vy = by - ay;
                const float len2 = vx * vx + vy * vy;
                float tt = 0.0f;
                if (len2 > 1e-6f)
                    tt = ((mx - ax) * vx + (my - ay) * vy) / len2;
                if (tt < 0.0f) tt = 0.0f;
                else if (tt > 1.0f) tt = 1.0f;
                const float dx = mx - (ax + vx * tt);
                const float dy = my - (ay + vy * tt);
                const float d2 = dx * dx + dy * dy;
                if (d2 < best) best = d2;
            }
            if (best <= tol * tol)
            {
                debugLogf(L"[plan-hit] 命中连线 %ls>%ls", l.from.c_str(), l.to.c_str());
                return l.to;
            }
        }
        return {};
    }

bool PyMfcHost::planCardRect(int id, const std::wstring& code, RECT& out) const
{
        out = RECT{0, 0, 0, 0};
        auto it = m_planCv.find(id);
        if (it == m_planCv.end() || it->second.mode != 1)
            return false;
        const PlanCanvasState& st = it->second;
        for (const xamlplan::PowerBox& b : st.plan.boxes)
        {
            if (_wcsicmp(b.code.c_str(), code.c_str()) != 0)
                continue;
            out.left = st.ox + static_cast<LONG>(lroundf(b.x * st.zoom));
            out.top = st.oy + static_cast<LONG>(lroundf(b.y * st.zoom));
            out.right = st.ox + static_cast<LONG>(lroundf((b.x + b.w) * st.zoom));
            out.bottom = st.oy + static_cast<LONG>(lroundf((b.y + b.h) * st.zoom));
            return true;
        }
        return false;
    }

bool PyMfcHost::saveControlPng(int id, const std::wstring& path)
{
        HWND c = ::GetDlgItem(m_hWnd, id);
        if (c == nullptr || !::IsWindow(c))
            return false;
        RECT wr = {};
        if (!::GetWindowRect(c, &wr))
            return false;
        const int w = wr.right - wr.left;
        const int hh = wr.bottom - wr.top;
        if (w <= 0 || hh <= 0)
            return false;

        HDC screen = ::GetDC(nullptr);
        HDC mem = ::CreateCompatibleDC(screen);
        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = w;
        bi.bmiHeader.biHeight = -hh;          // 自顶向下，与 wicWritePng 行序一致
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        HBITMAP bmp = ::CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
        HGDIOBJ oldBmp = (bmp != nullptr) ? ::SelectObject(mem, bmp) : nullptr;

        RECT full = { 0, 0, w, hh };
        HBRUSH bg = ::CreateSolidBrush(RGB(0xF3, 0xF5, 0xF8));
        ::FillRect(mem, &full, bg);
        ::DeleteObject(bg);

        // PW_RENDERFULLCONTENT(2)：内容渲染型窗口也能抓
        ::PrintWindow(c, mem, 2);
        ::GdiFlush();

        bool wrote = false;
        if (bits != nullptr)
            wrote = wicWritePng(path, w, hh, bits);

        if (oldBmp != nullptr)
            ::SelectObject(mem, oldBmp);
        if (bmp != nullptr)
            ::DeleteObject(bmp);
        ::DeleteDC(mem);
        ::ReleaseDC(nullptr, screen);
        return wrote;
    }

bool PyMfcHost::selectPlanItem(int id, const std::wstring& code)
{
        auto it = m_planCv.find(id);
        if (it == m_planCv.end() || it->second.mode != 1)
            return false;
        PlanCanvasState& st = it->second;
        const xamlplan::PowerBox* hit = nullptr;
        for (const xamlplan::PowerBox& b : st.plan.boxes)
        {
            if (_wcsicmp(b.code.c_str(), code.c_str()) == 0)
            {
                hit = &b;
                break;
            }
        }
        if (hit == nullptr)
            return false;

        st.sel = hit->code;
        RECT rc = {0, 0, 1, 1};
        HWND hwnd = ::GetDlgItem(m_hWnd, id);
        if (hwnd != nullptr)
            ::GetClientRect(hwnd, &rc);
        st.ox = static_cast<int>(lroundf(rc.right * 0.5f -
                                         (hit->x + hit->w * 0.5f) * st.zoom));
        st.oy = static_cast<int>(lroundf(rc.bottom * 0.5f -
                                         (hit->y + hit->h * 0.5f) * st.zoom));
        if (hwnd != nullptr)
            ::InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }

void PyMfcHost::firePlanSelect(int id, const std::wstring& /*code*/)
{
        auto it = m_click.find(id);
        if (it != m_click.end())
            invokePython(it->second);
    }

void PyMfcHost::firePlanDoubleClick(int id, const std::wstring& /*code*/)
{
        auto it = m_dblclick.find(id);
        if (it != m_dblclick.end())
            invokePython(it->second);
    }

void PyMfcHost::drawPlanCanvas(XamlCanvas& cv, int id, const RECT& rc)
{
        auto it = m_planCv.find(id);
        if (it == m_planCv.end())
            return;
        PlanCanvasState& st = it->second;
        cv.fillRect(rc, RGB(0xF3, 0xF5, 0xF8));
        // OpenGL 画布后端（PYARX_CANVAS_GL=1）：属于阶段一，先验证 WGL 上下文与呈现链路，
        // 矢量批处理（卡片/连线/字形图集）在此之后接入；失败自动落回下面的 GDI 路径。
        // OpenGL 画布后端（阶段二）尚未完成：字形重影、与 owner-draw 双缓冲互相覆盖、
        // 视口未按客户区取值，会导致闪烁与内容缺失。先不接管，保持 GDI 路径可用。
        // 完成字形 alpha 重建 + GL 独占画布清屏 + 客户区 viewport 后再打开下面这段。
#if 0
        if (pymfcgl::enabled() && st.mode == 1 && !st.mini)
        {
            HWND gh = ::GetDlgItem(m_hWnd, id);
            const int w = rc.right, h = rc.bottom;
            if (pymfcgl::drawPlan(gh, st.plan, st.zoom, st.ox, st.oy, st.sel, st.showCable, w, h))
                return;
        }
#endif
        if (st.mode == 0)
        {
            // 空画布：给一句提示，免得看起来像"没画出来"
            cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
            cv.text(rc, L"（配电画布：等待数据）", RGB(0x8A, 0x94, 0xA6), DT_CENTER, 10, L"Microsoft YaHei");
            return;
        }
        if (st.mode == 2)
        {
            xamlsvg::drawSvgScene(cv, st.svg, st.zoom, st.ox, st.oy);
            return;
        }
        if (st.mini)
        {
            // 与 HTML 的 drawMinimap 一致：底色 #FBFCFE + 1px #E4E7EC 边框，
            // 卡片画成实心 #CFE0F3 小块（不画文字/纸面），最后叠红色视口框
            cv.fillRect(rc, RGB(0xFB, 0xFC, 0xFE));
            cv.strokeRect(rc, RGB(0xE4, 0xE7, 0xEC), 1);
            const float w = static_cast<float>(st.plan.width);
            const float h = static_cast<float>(st.plan.height);
            if (w <= 0.0f || h <= 0.0f) return;
            const float pad = 4.0f;
            const float k = (std::min)((static_cast<float>(rc.right) - pad * 2) / w,
                                       (static_cast<float>(rc.bottom) - pad * 2) / h);
            if (k <= 0.0f) return;
            const float ox = (static_cast<float>(rc.right) - w * k) * 0.5f;
            const float oy = (static_cast<float>(rc.bottom) - h * k) * 0.5f;
            for (const xamlplan::PowerBox& b : st.plan.boxes)
            {
                const int x0 = static_cast<int>(lroundf(ox + b.x * k));
                const int y0 = static_cast<int>(lroundf(oy + b.y * k));
                const int x1 = static_cast<int>(lroundf(ox + (b.x + b.w) * k));
                const int y1 = static_cast<int>(lroundf(oy + (b.y + b.h) * k));
                RECT br = { x0, y0, (x1 > x0 + 1 ? x1 : x0 + 1), (y1 > y0 + 1 ? y1 : y0 + 1) };
                cv.fillRect(br, RGB(0xCF, 0xE0, 0xF3));
            }

            auto main = m_planCv.find(st.miniOf);
            if (main != m_planCv.end() && main->second.mode == 1)
            {
                const PlanCanvasState& ms = main->second;
                RECT mrc = { 0, 0, 1, 1 };
                if (HWND mh = ::GetDlgItem(m_hWnd, st.miniOf))
                    ::GetClientRect(mh, &mrc);
                const float vx1 = (-ms.ox) / (ms.zoom > 0 ? ms.zoom : 1.0f);
                const float vy1 = (-ms.oy) / (ms.zoom > 0 ? ms.zoom : 1.0f);
                const float vx2 = (static_cast<float>(mrc.right) - ms.ox) / (ms.zoom > 0 ? ms.zoom : 1.0f);
                const float vy2 = (static_cast<float>(mrc.bottom) - ms.oy) / (ms.zoom > 0 ? ms.zoom : 1.0f);
                RECT vr = { static_cast<int>(lroundf(ox + vx1 * k)), static_cast<int>(lroundf(oy + vy1 * k)),
                            static_cast<int>(lroundf(ox + vx2 * k)), static_cast<int>(lroundf(oy + vy2 * k)) };
                if (vr.right > vr.left && vr.bottom > vr.top)
                {
                    ::IntersectRect(&vr, &vr, &rc);
                    cv.strokeRect(vr, RGB(0xE1, 0x57, 0x59), 1);
                }
            }
            return;
        }
        drawPowerPlanContent(cv, st.plan, st.zoom, st.ox, st.oy, st.sel, st.showCable, &rc);

        if (st.detailVisible)
        {
            pymfcskin::ShadowBoxStyle sb;
            sb.pad = 14;
            sb.radius = 6;
            sb.dx = 0;
            sb.dy = 4;
            sb.blur = 12;
            sb.alpha = 26;
            sb.shadow = RGB(16, 24, 40);
            sb.bg = RGB(255, 255, 255);
            sb.back = RGB(255, 255, 255);
            sb.border = RGB(208, 213, 221);
            sb.borderW = 1;
            RECT outer = st.detailRc;
            ::InflateRect(&outer, sb.pad, sb.pad);
            pymfcskin::drawShadowBox(cv, outer, sb);
            for (const DetailText& dt : st.detailTexts)
            {
                cv.setTextStyle(dt.bold ? L"SemiBold" : L"Normal", L"Normal", L"NoWrap", L"Left");
                cv.text(dt.rc, dt.text, dt.color, DT_LEFT | DT_VCENTER | DT_SINGLELINE,
                        dt.size, L"Microsoft YaHei");
            }
        }
    }

void PyMfcHost::setGridCellSize(int id, int cellW, int cellH)
{
        if (cellW > 24) m_gvCellW[id] = cellW; else m_gvCellW.erase(id);
        if (cellH > 24) m_gvCellH[id] = cellH; else m_gvCellH.erase(id);
        HWND gh = ::GetDlgItem(m_hWnd, id);
        if (gh != nullptr)
            ::InvalidateRect(gh, nullptr, TRUE);
    }

void PyMfcHost::setCanvasCommands(int id, const std::wstring& cmds)
{
        m_cvCmd[id] = cmds;
        repaintCanvas(id);
    }

std::wstring PyMfcHost::getCanvasCommands(int id) const
{
        auto it = m_cvCmd.find(id);
        return it != m_cvCmd.end() ? it->second : std::wstring();
    }

int PyMfcHost::levelOf(int id) const
{
        auto i = m_zLevel.find(id);
        return i != m_zLevel.end() ? i->second : 0;
    }

int PyMfcHost::tabIndexOf(int id) const
{
        auto i = m_tabIndex.find(id);
        return i != m_tabIndex.end() ? i->second : -1;
    }

void PyMfcHost::noteLevel(int id, int level)
{
        if (level == 0)
            m_zLevel.erase(id);
        else
            m_zLevel[id] = level;
    }

void PyMfcHost::noteTabIndex(int id, int n)
{
        if (n < 0)
            m_tabIndex.erase(id);
        else
            m_tabIndex[id] = n;
    }

void PyMfcHost::raiseMany(const std::vector<int>& ids)
{
        int base = maxLevel();
        int n = 0;
        for (int id : ids) noteLevel(id, base + (++n));
        applyZOrder();
    }

void PyMfcHost::setLevel(int id, int level)
{
        noteLevel(id, level);
        applyZOrder();
    }

int PyMfcHost::getLevel(int id) const
{ return levelOf(id); }


int PyMfcHost::maxLevel() const
{
        int mx = 0;
        for (const auto& kv : m_zLevel)
            if (kv.second > mx) mx = kv.second;
        return mx;
    }

int PyMfcHost::minLevel() const
{
        int mn = 0;
        for (const auto& kv : m_zLevel)
            if (kv.second < mn) mn = kv.second;
        return mn;
    }

void PyMfcHost::bringToFront(int id)
{ setLevel(id, maxLevel() + 1); }

void PyMfcHost::bringToFrontMany(const std::vector<int>& ids)
{
    // 逐个 bringToFront：每次都要遍历子窗口重排 + RedrawWindow(全部子窗口)，
    // 10221 节点刷一次图这 5 次置顶要 120 ms；这里合并成一次重排
    int base = maxLevel();
    int n = 0;
    for (int id : ids) noteLevel(id, base + (++n));
    applyZOrder();
}


void PyMfcHost::sendToBack(int id)
{ setLevel(id, minLevel() - 1); }


void PyMfcHost::setTabIndex(int id, int n)
{
        noteTabIndex(id, n);
        applyTabProps();
    }

int PyMfcHost::getTabIndex(int id) const
{ return tabIndexOf(id); }


void PyMfcHost::noteCreated(int id)
{
        m_createSeq[id] = ++m_seqNext;
        if (HWND h = (m_hWnd != nullptr ? ::GetDlgItem(m_hWnd, id) : nullptr))
            ::SetPropW(h, L"PyMfcSeq",
                       reinterpret_cast<HANDLE>(static_cast<std::intptr_t>(m_seqNext)));
    }

void PyMfcHost::applyTabProps()
{
        for (HWND h = ::GetWindow(m_hWnd, GW_CHILD); h != nullptr;
             h = ::GetWindow(h, GW_HWNDNEXT))
        {
            auto ti = m_tabIndex.find(::GetDlgCtrlID(h));
            if (ti != m_tabIndex.end())
                ::SetPropW(h, L"PyMfcTabIndex",
                           reinterpret_cast<HANDLE>(static_cast<std::intptr_t>(ti->second)));
            else
                ::RemovePropW(h, L"PyMfcTabIndex");
        }
    }

void PyMfcHost::applyZOrder()
{
        struct ZItem
        {
            int level = 0;
            int seq = 0;
            HWND h = nullptr;
        };
        std::vector<ZItem> items;
        int seq = 0;
        for (HWND h = ::GetWindow(m_hWnd, GW_CHILD); h != nullptr;
             h = ::GetWindow(h, GW_HWNDNEXT))
        {
            ZItem z;
            z.level = levelOf(::GetDlgCtrlID(h));
            z.seq = ++seq;
            z.h = h;
            items.push_back(z);
        }
        const std::vector<HWND> before = [&]() {
            std::vector<HWND> v;
            v.reserve(items.size());
            for (const ZItem& z : items) v.push_back(z.h);
            return v;
        }();
        std::sort(items.begin(), items.end(), [](const ZItem& a, const ZItem& b) {
            if (a.level != b.level)
                return a.level < b.level;
            return a.seq < b.seq;
        });
        // 次序没变就别重排、更别整窗重画（重复 rebuild 时这里是纯浪费）
        bool same = (items.size() == before.size());
        for (size_t i = 0; same && i < items.size(); ++i)
            if (items[i].h != before[i]) same = false;
        if (same)
            return;
        for (const ZItem& z : items)
            ::SetWindowPos(z.h, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        // 换了叠放次序必须整片重画：两块重叠时，谁从底下翻上来，
        // 原来压着它的那块留下的像素不会自己消失，看上去就像"层次没变"。
        ::RedrawWindow(m_hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
    }

void PyMfcHost::applyLevelsAndTabs()
{
        applyTabProps();
        applyZOrder();
    }

void PyMfcHost::repaintCanvas(int id)
{
        RECT rc = {0, 0, 1, 1};
        if (HWND h = ::GetDlgItem(m_hWnd, id))
        {
            ::GetClientRect(h, &rc);
            ::InvalidateRect(h, nullptr, TRUE);
        }
        else
        {
            ::InvalidateRect(m_hWnd, nullptr, TRUE);
        }
        if (rc.right <= 0 || rc.bottom <= 0)
            rc = RECT{0, 0, 1, 1};
        const auto it = m_cvCmd.find(id);
        m_cvItems[id] = pymfcskin::qcParse(it != m_cvCmd.end() ? it->second : std::wstring(), rc);
        // 重解析后下标可能整体变了（删过图元），选中集合夹到合法范围
        auto& sel = m_cvSel[id];
        const int n = static_cast<int>(m_cvItems[id].size());
        for (auto i = sel.begin(); i != sel.end();)
        {
            if (*i < 0 || *i >= n)
                i = sel.erase(i);
            else
                ++i;
        }
    }

void PyMfcHost::setCanvasEditable(int id, bool on)
{
        if (on)
            m_cvEdit[id] = true;
        else
            m_cvEdit.erase(id);
        repaintCanvas(id);
    }

bool PyMfcHost::getCanvasEditable(int id) const
{
        return m_cvEdit.count(id) != 0;
    }

int PyMfcHost::canvasItemCount(int id) const
{
        auto it = m_cvItems.find(id);
        return it != m_cvItems.end() ? static_cast<int>(it->second.size()) : 0;
    }

void PyMfcHost::canvasItem(int id, int index, int& outOp, RECT& outBox) const
{
        outOp = 0;
        outBox = RECT{0, 0, 0, 0};
        auto it = m_cvItems.find(id);
        if (it == m_cvItems.end() || index < 0 || index >= static_cast<int>(it->second.size()))
            return;
        outBox = it->second[index].box;
        outOp = 1;   // 只表示"存在"，命令名由 getCanvasItemOp 取
    }

std::wstring PyMfcHost::canvasItemOp(int id, int index) const
{
        auto it = m_cvItems.find(id);
        if (it == m_cvItems.end() || index < 0 || index >= static_cast<int>(it->second.size()))
            return std::wstring();
        return it->second[index].f.empty() ? std::wstring() : it->second[index].f[0];
    }

int PyMfcHost::canvasHitTest(int id, POINT pt) const
{
        auto it = m_cvItems.find(id);
        if (it == m_cvItems.end())
            return -1;
        const std::vector<pymfcskin::QcPiece>& v = it->second;
        for (int i = static_cast<int>(v.size()) - 1; i >= 0; --i)
        {
            if (!v[i].selectable)
                continue;
            RECT b = v[i].box;
            ::InflateRect(&b, 2, 2);
            if (pt.x >= b.left && pt.x < b.right && pt.y >= b.top && pt.y < b.bottom)
                return i;
        }
        return -1;
    }

std::vector<int> PyMfcHost::getCanvasSelection(int id) const
{
        auto it = m_cvSel.find(id);
        return it != m_cvSel.end() ? it->second : std::vector<int>();
    }

void PyMfcHost::fireCanvasSelect(int id)
{
        auto it = m_cvSelCb.find(static_cast<UINT>(id));
        if (it != m_cvSelCb.end())
            invokePython(it->second);
    }

void PyMfcHost::setCanvasSelection(int id, const std::vector<int>& idx)
{
        std::vector<int> v;
        const int n = canvasItemCount(id);
        for (int i : idx)
        {
            if (i >= 0 && i < n && m_cvItems[id][i].selectable)
                v.push_back(i);
        }
        m_cvSel[id] = v;
        repaintCanvas(id);
        fireCanvasSelect(id);
    }

void PyMfcHost::clearCanvasSelection(int id)
{
        m_cvSel[id].clear();
        repaintCanvas(id);
        fireCanvasSelect(id);
    }

void PyMfcHost::onCanvasSelect(int id, py::object cb)
{ m_cvSelCb[static_cast<UINT>(id)] = std::move(cb); }


int PyMfcHost::removeCanvasItems(int id, const std::vector<int>& idx)
{
        auto it = m_cvCmd.find(id);
        if (it == m_cvCmd.end())
            return 0;
        std::vector<bool> drop;
        if (m_cvItems.count(id))
        {
            drop.assign(m_cvItems[id].size(), false);
            for (int i : idx)
            {
                if (i >= 0 && i < static_cast<int>(drop.size()))
                    drop[i] = true;
            }
        }
        std::vector<int> keep;
        auto itemsIt = m_cvItems.find(id);
        const int n = itemsIt != m_cvItems.end() ? static_cast<int>(itemsIt->second.size()) : 0;
        for (int i = 0; i < n; ++i)
        {
            if (i < static_cast<int>(drop.size()) && drop[i])
                continue;
            keep.push_back(i);
        }

        // 用"解析后的片段顺序"重建：只保留解析器认得出来的片段（与绘制一致）
        std::wstring out;
        if (itemsIt != m_cvItems.end())
        {
            for (int i : keep)
            {
                if (!out.empty())
                    out += L"; ";
                out += itemsIt->second[i].raw;
            }
        }
        m_cvCmd[id] = out;
        m_cvSel[id].clear();
        const int removed = n - static_cast<int>(keep.size());
        repaintCanvas(id);
        fireCanvasSelect(id);
        return removed;
    }

int PyMfcHost::removeCanvasSelection(int id)
{
        const std::vector<int> sel = getCanvasSelection(id);
        return sel.empty() ? 0 : removeCanvasItems(id, sel);
    }

void PyMfcHost::setLayerOpacity(int id, int alpha)
{
        if (alpha < 0) alpha = 0;
        if (alpha > 255) alpha = 255;
        if (alpha >= 255)
            m_layerAlpha.erase(id);
        else
            m_layerAlpha[id] = alpha;
        HWND h = ::GetDlgItem(m_hWnd, id);
        ::InvalidateRect(h != nullptr ? h : m_hWnd, nullptr, TRUE);
    }

int PyMfcHost::getLayerOpacity(int id) const
{
        auto it = m_layerAlpha.find(id);
        return it != m_layerAlpha.end() ? it->second : 255;
    }

void PyMfcHost::addListItem(int id, const std::wstring& line)
{
        std::vector<std::wstring> f(3);
        int k = 0;
        for (wchar_t ch : line)
        {
            if (ch == L'|' || ch == L'\t')
            {
                if (k < 2) ++k;
                continue;
            }
            f[k].push_back(ch);
        }
        m_lvT[id].push_back(f[0]);
        m_lvS[id].push_back(f[1]);
        m_lvV[id].push_back(f[2]);
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::clearListItems(int id)
{
        m_lvT[id].clear();
        m_lvS[id].clear();
        m_lvV[id].clear();
        m_lvSel[id] = -1;
        m_lvScroll[id] = 0;
        m_lvOver[id] = 0;
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
    }

int PyMfcHost::getListItemCount(int id) const
{
        auto it = m_lvT.find(id);
        return it != m_lvT.end() ? static_cast<int>(it->second.size()) : 0;
    }

std::wstring PyMfcHost::getListItem(int id, int index) const
{
        auto it = m_lvT.find(id);
        if (it == m_lvT.end() || index < 0 || index >= static_cast<int>(it->second.size()))
            return {};
        std::wstring s = it->second[index];
        auto s2 = m_lvS.find(id);
        if (s2 != m_lvS.end() && index < static_cast<int>(s2->second.size()) && !s2->second[index].empty())
            s += L"|" + s2->second[index];
        auto v2 = m_lvV.find(id);
        if (v2 != m_lvV.end() && index < static_cast<int>(v2->second.size()) && !v2->second[index].empty())
            s += L"|" + v2->second[index];
        return s;
    }

int PyMfcHost::getListSelected(int id) const
{
        auto it = m_lvSel.find(id);
        return it != m_lvSel.end() ? it->second : -1;
    }

void PyMfcHost::setListSelected(int id, int index)
{
        // 序号一律夹到合法区间：越界值会让绘制层按错误下标取数据
        const int cnt = getListItemCount(id);
        if (cnt > 0)
        {
            if (index < 0) index = 0;
            if (index > cnt - 1) index = cnt - 1;
        }
        else
        {
            index = -1;
        }
        m_lvSel[id] = index;
        if (index < 0)
            return;
        // 选中项要保证在可见窗口里：滚动过去
        int rowH = lvRowH(id);
        int rowIdx = index;
        const auto kg = m_skinKinds.find(id);
        if (kg != m_skinKinds.end() && kg->second == kSkinBoxGridView)
        {
            int cols = 1, cw = 92, ch = 96;
            gvMetrics(id, cols, cw, ch);
            rowIdx = index / cols;
            rowH = ch;
        }
        const int top = rowIdx * rowH;
        const int max = lvMaxScroll(id);
        int sc = m_lvScroll.count(id) ? m_lvScroll[id] : 0;
        if (top < sc)
            sc = top;
        else if (top + rowH > sc + max + rowH)
            sc = top - max;
        if (sc > max) sc = max;
        if (sc < 0) sc = 0;
        m_lvScroll[id] = sc;
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
    }

int PyMfcHost::getListScroll(int id) const
{
        auto it = m_lvScroll.find(id);
        return it != m_lvScroll.end() ? it->second : 0;
    }

void PyMfcHost::setListScroll(int id, int pos)
{
        m_lvScroll[id] = pos;
        lvClampScroll(id);
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
    }

int PyMfcHost::flickTick(int id)
{
        if (m_flBase.count(id))
            return flickTickFlickable(id);
        auto it = m_flickVel.find(id);
        int v = (it != m_flickVel.end()) ? it->second : 0;
        int over = m_lvOver.count(id) ? m_lvOver[id] : 0;
        if (v == 0 && over == 0)
            return 0;

        const ULONGLONG now = ::GetTickCount64();
        auto st = m_flickStamp.find(id);
        const int dt = (st != m_flickStamp.end() && now > st->second)
                           ? static_cast<int>(now - st->second) : 16;
        m_flickStamp[id] = now;
        const int adv = v * (dt > 0 ? dt : 16) / 1000;

        const int maxSc = lvMaxScroll(id);
        const int raw = getListScroll(id) + over + adv;
        int sc = raw;
        if (raw < 0)
        {
            sc = 0;
            over = raw;                     // 顶端越界：值越小，内容向下多滑出越多
        }
        else if (raw > maxSc)
        {
            sc = maxSc;
            over = raw - maxSc;
        }
        else
        {
            over = 0;
        }
        if (over < -96) over = -96;
        if (over > 96) over = 96;

        if (over != 0)
        {
            // 触边：速度被吃掉，只留弹性收敛（每帧 0.86 收，收到 2px 内归零）
            v = 0;
            over = static_cast<int>(over * 0.86);
            if (over > -2 && over < 2)
                over = 0;
        }
        else
        {
            v = static_cast<int>(v * 0.92);
            if (v > -40 && v < 40)
                v = 0;
        }

        setListScroll(id, sc);
        m_lvOver[id] = over;
        m_flickVel[id] = v;
        return (v != 0 || over != 0) ? 1 : 0;
    }

void PyMfcHost::flickMetrics(int id, int& viewW, int& viewH, int& cw, int& ch,
                      int& minX, int& minY) const
{
        viewW = viewH = 0;
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h != nullptr)
        {
            RECT rc = {};
            ::GetClientRect(h, &rc);
            viewW = rc.right;
            viewH = rc.bottom;
        }
        auto itCW = m_flCW.find(id);
        auto itCH = m_flCH.find(id);
        cw = itCW != m_flCW.end() ? itCW->second : 0;
        ch = itCH != m_flCH.end() ? itCH->second : 0;
        auto itBase = m_flBase.find(id);
        if (itBase != m_flBase.end())
            for (const auto& kv : itBase->second)
            {
                if (kv.second.right > cw) cw = kv.second.right;
                if (kv.second.bottom > ch) ch = kv.second.bottom;
            }
        if (cw < viewW) cw = viewW;
        if (ch < viewH) ch = viewH;
        minX = viewW - cw;          // 负值：可向左推的距离
        minY = viewH - ch;
    }

void PyMfcHost::flickApply(int id)
{
        auto itBase = m_flBase.find(id);
        if (itBase == m_flBase.end())
            return;
        const POINT off = m_flOff.count(id) ? m_flOff[id] : POINT{0, 0};
        HWND hfl = ::GetDlgItem(m_hWnd, id);
        if (hfl == nullptr)
            return;
        for (const auto& kv : itBase->second)
        {
            HWND hk = ::GetDlgItem(hfl, kv.first);
            if (hk == nullptr)
                continue;
            const RECT& b = kv.second;
            ::SetWindowPos(hk, nullptr, b.left + off.x, b.top + off.y,
                           b.right - b.left, b.bottom - b.top,
                           SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

void PyMfcHost::setFlickContent(int id, const std::vector<int>& kids, int cw, int ch)
{
        HWND hfl = ::GetDlgItem(m_hWnd, id);
        if (hfl == nullptr)
            return;
        m_flKids[id] = kids;
        m_flBase[id].clear();
        for (int kid : kids)
        {
            HWND hk = ::GetDlgItem(m_hWnd, kid);
            if (hk == nullptr)
                continue;
            RECT cr = {};
            ::GetWindowRect(hk, &cr);
            POINT tl = {cr.left, cr.top};
            POINT br2 = {cr.right, cr.bottom};
            ::MapWindowPoints(HWND_DESKTOP, hfl, &tl, 1);
            ::MapWindowPoints(HWND_DESKTOP, hfl, &br2, 1);
            RECT base = {tl.x, tl.y, br2.x, br2.y};
            m_flBase[id][kid] = base;
            ::SetParent(hk, hfl);            // 变成视口子窗 → 超出视口的部分自然被裁掉
            ::SetWindowPos(hk, HWND_TOP, base.left, base.top,
                           base.right - base.left, base.bottom - base.top,
                           SWP_NOACTIVATE);
        }
        m_flCW[id] = cw;
        m_flCH[id] = ch;
        m_flOff[id] = POINT{0, 0};
        m_flVel[id] = POINT{0, 0};
        flickApply(id);
        ::InvalidateRect(hfl, nullptr, TRUE);
    }

void PyMfcHost::clearFlickContent(int id)
{
        HWND hfl = ::GetDlgItem(m_hWnd, id);
        auto it = m_flBase.find(id);
        if (it != m_flBase.end() && hfl != nullptr)
            for (const auto& kv : it->second)
                if (HWND hk = ::GetDlgItem(hfl, kv.first))
                {
                    RECT r = kv.second;
                    ::MapWindowPoints(hfl, m_hWnd, reinterpret_cast<LPPOINT>(&r), 2);
                    ::SetParent(hk, m_hWnd);
                    ::SetWindowPos(hk, nullptr, r.left, r.top, r.right - r.left,
                                   r.bottom - r.top, SWP_NOZORDER | SWP_NOACTIVATE);
                    ::ShowWindow(hk, SW_SHOW);
                }
        m_flKids.erase(id);
        m_flBase.erase(id);
        m_flOff.erase(id);
        m_flVel.erase(id);
    }

void PyMfcHost::setFlickOffset(int id, int x, int y)
{
        int vw, vh, cw, ch, minX, minY;
        flickMetrics(id, vw, vh, cw, ch, minX, minY);
        if (x > 0) x = 0;
        if (x < minX) x = minX;
        if (y > 0) y = 0;
        if (y < minY) y = minY;
        m_flOff[id] = POINT{x, y};
        m_flVel[id] = POINT{0, 0};
        flickApply(id);
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
    }

int PyMfcHost::getFlickOffsetX(int id) const
{
        auto it = m_flOff.find(id);
        return it != m_flOff.end() ? it->second.x : 0;
    }

int PyMfcHost::getFlickOffsetY(int id) const
{
        auto it = m_flOff.find(id);
        return it != m_flOff.end() ? it->second.y : 0;
    }

int PyMfcHost::flickTickFlickable(int id)
{
        if (!m_flOff.count(id))
            return 0;
        const ULONGLONG now = ::GetTickCount64();
        auto st = m_flStamp.find(id);
        const int dt = (st != m_flStamp.end() && now > st->second)
                           ? static_cast<int>(now - st->second) : 16;
        m_flStamp[id] = now;
        auto vt = m_flVel.find(id);
        int vx = vt != m_flVel.end() ? vt->second.x : 0;
        int vy = vt != m_flVel.end() ? vt->second.y : 0;
        int vw, vh, cw, ch, minX, minY;
        flickMetrics(id, vw, vh, cw, ch, minX, minY);
        POINT& off = m_flOff[id];
        off.x += vx * (dt > 0 ? dt : 16) / 1000;
        off.y += vy * (dt > 0 ? dt : 16) / 1000;
        if (off.x > 0)
        {
            off.x = static_cast<int>(off.x * 0.86);
            if (off.x < 3) off.x = 0;
            vx = 0;
        }
        else if (off.x < minX)
        {
            off.x = minX + static_cast<int>((off.x - minX) * 0.86);
            if (off.x - minX > -3) off.x = minX;
            vx = 0;
        }
        else
        {
            vx = static_cast<int>(vx * 0.92);
            if (vx > -40 && vx < 40) vx = 0;
        }
        if (off.y > 0)
        {
            off.y = static_cast<int>(off.y * 0.86);
            if (off.y < 3) off.y = 0;
            vy = 0;
        }
        else if (off.y < minY)
        {
            off.y = minY + static_cast<int>((off.y - minY) * 0.86);
            if (off.y - minY > -3) off.y = minY;
            vy = 0;
        }
        else
        {
            vy = static_cast<int>(vy * 0.92);
            if (vy > -40 && vy < 40) vy = 0;
        }
        if (vt != m_flVel.end())
            vt->second = POINT{vx, vy};
        flickApply(id);
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
        // 还在动，或者还停在越界位置（要继续收敛）才继续跑
        const bool busy = vx != 0 || vy != 0 || off.x > 0 || off.x < minX ||
                          off.y > 0 || off.y < minY;
        return busy ? 1 : 0;
    }

void PyMfcHost::pageRegister(int id)
{
        if (!m_pgIdx.count(id))
            m_pgIdx[id] = 0;
    }

void PyMfcHost::clearPages(int id)
{
        // 先把所有页的子控件还原成原样（可见、位置回到基准）
        auto it = m_pgPages.find(id);
        if (it != m_pgPages.end())
            for (auto& pg : it->second)
                for (int kid : pg)
                {
                    HWND hk = ::GetDlgItem(m_hWnd, kid);
                    if (hk != nullptr)
                    {
                        auto b = m_pgBase[id].find(kid);
                        if (b != m_pgBase[id].end())
                            ::SetWindowPos(hk, nullptr, b->second.left, b->second.top,
                                           b->second.right - b->second.left,
                                           b->second.bottom - b->second.top,
                                           SWP_NOZORDER | SWP_NOACTIVATE);
                        ::ShowWindow(hk, SW_SHOW);
                    }
                }
        if (it != m_pgPages.end())
            it->second.clear();
        m_pgIdx[id] = 0;
        m_pgAnimDur.erase(id);
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
    }

RECT PyMfcHost::pageHostRect(int id) const
{
        RECT rc = { 0, 0, 0, 0 };
        HWND hh = ::GetDlgItem(m_hWnd, id);
        if (hh != nullptr)
        {
            ::GetWindowRect(hh, &rc);
            ::MapWindowPoints(nullptr, m_hWnd, reinterpret_cast<LPPOINT>(&rc), 2);
        }
        return rc;
    }

void PyMfcHost::pageLayout(int id, int which, int offset, bool show)
{
        auto it = m_pgPages.find(id);
        if (it == m_pgPages.end() || which < 0 || which >= static_cast<int>(it->second.size()))
            return;
        const RECT hr = pageHostRect(id);
        // 页内容必须压在页容器上面。z 序用 HWND_TOP：每次插入都落到兄弟链最顶，
        // 所以按"视觉自底向上"的顺序摆（先插的在下、后插的在上）：
        //   Level 小的先插（Level 大的压在上面）；同级按声明顺序，后声明的压在上面。
        // 早先这里用 SetWindowPos(hk, 页容器, …)，而 Win32 的 insert-after 语义是
        // "排在它后面"——结果是页内容被插到容器下面，容器的opaque底板把整页盖成一片空。
        const auto& kids = it->second[which];
        std::vector<int> order(kids.begin(), kids.end());
        std::sort(order.begin(), order.end(), [&](int a, int b) {
            const int la = levelOf(a);
            const int lb = levelOf(b);
            if (la != lb)
                return la < lb;      // Level 小的先插（先插 = 更靠下）
            return a < b;            // 同级按声明顺序：后声明的后插、压在上面
        });
        for (int kid : order)
        {
            HWND hk = ::GetDlgItem(m_hWnd, kid);
            if (hk == nullptr)
                continue;
            auto b = m_pgBase[id].find(kid);
            if (b == m_pgBase[id].end())
            {
                RECT cr = { 0, 0, 0, 0 };
                ::GetWindowRect(hk, &cr);
                ::MapWindowPoints(nullptr, m_hWnd, reinterpret_cast<LPPOINT>(&cr), 2);
                m_pgBase[id][kid] = cr;
                b = m_pgBase[id].find(kid);
            }
            // 页内容必须压在页容器上面：容器先建，落在 z 序顶端会把整页内容盖住。
            // 位置 = 基准矩形相对页容器的偏移 + 滑动偏移，尺寸保持 XAML 里声明的尺寸：
            // 早先这里把子控件拉成"整页大小"，切走再切回就再也回不到初始样子。
            const RECT& base = b->second;
            const int bx = base.left - hr.left;
            const int by = base.top - hr.top;
            ::SetWindowPos(hk, HWND_TOP,
                           hr.left + bx + offset, hr.top + by,
                           base.right - base.left, base.bottom - base.top,
                           SWP_NOACTIVATE);
            ::ShowWindow(hk, show ? SW_SHOW : SW_HIDE);
            if (show)
                ::InvalidateRect(hk, nullptr, TRUE);
        }
    }

void PyMfcHost::addPage(int id, const std::vector<int>& kids)
{
        // 记录基准坐标（以后切页会覆盖成容器矩形 + 偏移）
        for (int kid : kids)
        {
            HWND hk = ::GetDlgItem(m_hWnd, kid);
            if (hk == nullptr)
                continue;
            RECT cr = { 0, 0, 0, 0 };
            ::GetWindowRect(hk, &cr);
            ::MapWindowPoints(nullptr, m_hWnd, reinterpret_cast<LPPOINT>(&cr), 2);
            m_pgBase[id][kid] = cr;
        }
        const int idx = static_cast<int>(m_pgPages[id].size());
        m_pgPages[id].push_back(kids);
        const int cur = m_pgIdx.count(id) ? m_pgIdx[id] : 0;
        pageLayout(id, idx, 0, idx == cur);
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
    }

int PyMfcHost::viewCount(int id) const
{
        auto it = m_pgPages.find(id);
        return it != m_pgPages.end() ? static_cast<int>(it->second.size()) : 0;
    }

int PyMfcHost::viewIndex(int id) const
{
        auto it = m_pgIdx.find(id);
        return it != m_pgIdx.end() ? it->second : 0;
    }

void PyMfcHost::viewSetIndex(int id, int index)
{
        const int cnt = viewCount(id);
        if (cnt <= 0)
        {
            m_pgIdx[id] = index;
            return;
        }
        if (index < 0) index = 0;
        if (index >= cnt) index = cnt - 1;
        const int old = viewIndex(id);
        m_pgIdx[id] = index;
        m_pgAnimDur.erase(id);
        ::KillTimer(::GetDlgItem(m_hWnd, id), kPageTimerId);
        pageLayout(id, old, 0, false);
        pageLayout(id, index, 0, true);
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::pageSlide(int id, int which, int offset)
{
        auto it = m_pgPages.find(id);
        if (it == m_pgPages.end() || which < 0 || which >= static_cast<int>(it->second.size()))
            return;
        const RECT hr = pageHostRect(id);
        const auto& kids = it->second[which];
        if (kids.empty())
            return;
        HDWP dwp = ::BeginDeferWindowPos(static_cast<int>(kids.size()));
        for (int kid : kids)
        {
            HWND hk = ::GetDlgItem(m_hWnd, kid);
            if (hk == nullptr)
                continue;
            auto b = m_pgBase[id].find(kid);
            if (b == m_pgBase[id].end())
                continue;
            const RECT& base = b->second;
            const int bx = base.left - hr.left;
            const int by = base.top - hr.top;
            const int w = base.right - base.left;
            const int h = base.bottom - base.top;
            if (dwp != nullptr)
            {
                // 不能用 SWP_NOREDRAW：它连"把窗口像素搬到新位置"都不做，
                // 子窗会在原位留一块残影、新位置空着，父窗一重画就闪成一片。
                dwp = ::DeferWindowPos(dwp, hk, nullptr, hr.left + bx + offset, hr.top + by, w, h,
                                       SWP_NOZORDER | SWP_NOACTIVATE);
                if (dwp != nullptr)
                    continue;
            }
            // 批量句柄拿不到时退回逐个移动（功能优先，慢一点也要动）
            ::SetWindowPos(hk, nullptr, hr.left + bx + offset, hr.top + by, w, h,
                           SWP_NOZORDER | SWP_NOACTIVATE);
        }
        if (dwp != nullptr)
            ::EndDeferWindowPos(dwp);
    }

void PyMfcHost::viewAnimate(int id, int index, int durMs)
{
        const int cnt = viewCount(id);
        if (cnt <= 0)
        {
            m_pgIdx[id] = index;
            return;
        }
        if (index < 0) index = 0;
        if (index >= cnt) index = cnt - 1;
        const int old = viewIndex(id);
        if (old == index)
            return;
        if (durMs <= 0)
        {
            viewSetIndex(id, index);
            return;
        }
        m_pgAnimFrom[id] = old;
        m_pgAnimTo[id] = index;
        m_pgAnimStart[id] = ::GetTickCount64();
        m_pgAnimDur[id] = durMs;
        m_pgIdx[id] = index;
        HWND hh = ::GetDlgItem(m_hWnd, id);
        if (hh != nullptr)
            ::SetTimer(hh, kPageTimerId, 16, nullptr);
    }

int PyMfcHost::pageTick(int id)
{
        if (!m_pgAnimDur.count(id))
            return 0;
        const int dur = m_pgAnimDur[id];
        const ULONGLONG now = ::GetTickCount64();
        const int t = static_cast<int>(now - m_pgAnimStart[id]);
        const RECT hr = pageHostRect(id);
        const int w = hr.right - hr.left;
        if (t >= dur)
        {
            pageLayout(id, m_pgAnimFrom[id], 0, false);
            pageLayout(id, m_pgAnimTo[id], 0, true);
            m_pgAnimDur.erase(id);
            ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);
            return 0;
        }
        // 缓动 outCubic：1-(1-x)^3
        const double x = static_cast<double>(t) / dur;
        const double e = 1.0 - (1.0 - x) * (1.0 - x) * (1.0 - x);
        const int off = static_cast<int>(w * e);
        pageSlide(id, m_pgAnimFrom[id], -off);
        pageSlide(id, m_pgAnimTo[id], w - off);
        // 只补容器空出来的条纹，不擦底、不逐个子窗重画
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, FALSE);
        return 1;
    }

double PyMfcHost::easeAt(const std::string& ease, double t)
{
        if (t <= 0.0) return 0.0;
        if (t >= 1.0) return 1.0;
        auto pow2 = [](double v) { return v * v; };
        auto pow3 = [](double v) { return v * v * v; };
        if (ease == "linear") return t;
        if (ease == "inQuad") return pow2(t);
        if (ease == "outQuad") return 1.0 - pow2(1.0 - t);
        if (ease == "inOutQuad") return t < 0.5 ? 2.0 * pow2(t) : 1.0 - 2.0 * pow2(1.0 - t);
        if (ease == "inCubic") return pow3(t);
        if (ease == "outCubic") return 1.0 - pow3(1.0 - t);
        if (ease == "inOutCubic")
            return t < 0.5 ? 4.0 * pow3(t) : 1.0 - 4.0 * pow3(1.0 - t);
        if (ease == "outBack")
        {
            const double c1 = 1.70158, c3 = c1 + 1.0;
            const double u = t - 1.0;
            return 1.0 + c3 * u * u * u + c1 * u * u;
        }
        if (ease == "outElastic")
        {
            const double c4 = (2.0 * 3.14159265358979) / 3.0;
            return std::pow(2.0, -10.0 * t) * std::sin((t * 10.0 - 0.75) * c4) + 1.0;
        }
        if (ease == "spring")
        {
            // 临界阻尼附近的解析解：先冲过 1 再回落，收尾为 1
            const double w = 12.0;
            return 1.0 - std::exp(-6.0 * t) * (std::cos(w * t) + 0.5 * std::sin(w * t));
        }
        if (ease.compare(0, 7, "bezier(") == 0)
        {
            double x1 = 0.25, y1 = 0.1, x2 = 0.25, y2 = 1.0;
            double v[4] = {0, 0, 0, 0};
            std::sscanf(ease.c_str(), "bezier(%lf,%lf,%lf,%lf)", &v[0], &v[1], &v[2], &v[3]);
            x1 = v[0]; y1 = v[1]; x2 = v[2]; y2 = v[3];
            auto bez = [](double a, double b, double u) {
                const double m = 1.0 - u;
                return 3.0 * m * m * u * a + 3.0 * m * u * u * b + u * u * u;
            };
            double lo = 0.0, hi = 1.0, u = t;
            for (int i = 0; i < 24; ++i)   // 二分求参数，稳过牛顿
            {
                u = (lo + hi) / 2.0;
                const double x = bez(x1, x2, u);
                if (x < t) lo = u; else hi = u;
            }
            return bez(y1, y2, (lo + hi) / 2.0);
        }
        return t;
    }

COLORREF PyMfcHost::lerpColor(COLORREF a, COLORREF b, double p)
{
        auto ch = [](COLORREF c, int shift) { return (int)((c >> shift) & 0xFF); };
        auto mix = [&](int shift) {
            const double v = ch(a, shift) + (ch(b, shift) - ch(a, shift)) * p;
            return (int)(v < 0 ? 0 : (v > 255 ? 255 : v));
        };
        return RGB(mix(0), mix(8), mix(16));
    }

void PyMfcHost::ensureAnimWnd()
{
        if (m_animWnd != nullptr)
            return;
        WNDCLASSEXW wc = {sizeof(wc)};
        wc.style = 0;
        wc.lpfnWndProc = &PyMfcHost::animHostProc;
        wc.hInstance = ::GetModuleHandleW(nullptr);
        wc.lpszClassName = kAnimHostClass;
        ::RegisterClassExW(&wc);   // 已注册会失败，忽略即可
        m_animWnd = ::CreateWindowExW(0, kAnimHostClass, L"", 0, 0, 0, 0, 0,
                                      HWND_MESSAGE, nullptr, wc.hInstance, this);
    }

void PyMfcHost::animAdd(const PyMfcHost::AnimTrack& tr)
{
        if (m_anims.empty())
            m_animStartMs = static_cast<DWORD>(::GetTickCount64());
        m_anims.push_back(tr);
        ensureAnimWnd();
        if (!m_animTickOn && m_animWnd != nullptr)
        {
            ::SetTimer(m_animWnd, kAnimTimerId, 16, nullptr);   // ~60fps
            m_animTickOn = true;
        }
    }

void PyMfcHost::repairSiblingsOf(HWND moved, const RECT& oldRc, const RECT& newRc)
{
        RECT u = {
            oldRc.left < newRc.left ? oldRc.left : newRc.left,
            oldRc.top < newRc.top ? oldRc.top : newRc.top,
            oldRc.right > newRc.right ? oldRc.right : newRc.right,
            oldRc.bottom > newRc.bottom ? oldRc.bottom : newRc.bottom,
        };
        struct Ctx
        {
            HWND self;
            HWND moved;
            RECT r;
        } ctx = {m_hWnd, moved, u};
        ::EnumChildWindows(m_hWnd, [](HWND c, LPARAM lp) -> BOOL {
            Ctx* p = reinterpret_cast<Ctx*>(lp);
            if (c == p->moved || ::GetParent(c) != p->self)
                return TRUE;
            RECT cr = {};
            ::GetWindowRect(c, &cr);
            POINT a = {cr.left, cr.top};
            POINT b = {cr.right, cr.bottom};
            ::MapWindowPoints(HWND_DESKTOP, p->self, &a, 1);
            ::MapWindowPoints(HWND_DESKTOP, p->self, &b, 1);
            RECT rr = {a.x, a.y, b.x, b.y};
            RECT in = {};
            if (::IntersectRect(&in, &rr, &p->r))
                ::RedrawWindow(c, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
            return TRUE;
        }, reinterpret_cast<LPARAM>(&ctx));
    }

void PyMfcHost::applyAnimValue(const PyMfcHost::AnimTrack& tr, double v, COLORREF c)
{
        HWND h = ::GetDlgItem(m_hWnd, tr.id);
        if (tr.color)
        {
            if (m_ctrlBack.count(tr.id) && m_ctrlBack[tr.id] == c)
                return;                       // 同色不重画
            m_ctrlBack[tr.id] = c;
            if (h != nullptr)
                ::InvalidateRect(h, nullptr, TRUE);
            return;
        }
        m_animApplying = true;
        if (tr.prop == L"X" || tr.prop == L"Y" || tr.prop == L"Width" || tr.prop == L"Height")
        {
            if (h != nullptr)
            {
                RECT rc = {};
                ::GetWindowRect(h, &rc);
                POINT tl = {rc.left, rc.top};
                POINT br = {rc.right, rc.bottom};
                ::MapWindowPoints(HWND_DESKTOP, m_hWnd, &tl, 1);
                ::MapWindowPoints(HWND_DESKTOP, m_hWnd, &br, 1);
                int x = tl.x, y = tl.y;
                int w = br.x - tl.x, hgt = br.y - tl.y;
                if (tr.prop == L"X") x = static_cast<int>(v);
                else if (tr.prop == L"Y") y = static_cast<int>(v);
                else if (tr.prop == L"Width") w = static_cast<int>(v);
                else hgt = static_cast<int>(v);
                if (w < 1) w = 1;
                if (hgt < 1) hgt = 1;
                ::SetWindowPos(h, nullptr, x, y, w, hgt,
                               SWP_NOZORDER | SWP_NOACTIVATE);
                // 老矩形与新矩形的并集都可能"变回背景/露出兄弟"，交给兄弟重画
                RECT oldRc = { tl.x, tl.y, tl.x + (br.x - tl.x), tl.y + (br.y - tl.y) };
                RECT newRc = { x, y, x + w, y + hgt };
                repairSiblingsOf(h, oldRc, newRc);
                ::InvalidateRect(m_hWnd, &oldRc, TRUE);
                ::InvalidateRect(m_hWnd, &newRc, TRUE);
            }
        }
        else if (tr.prop == L"Value")
        {
            int kind = 0;
            auto it = m_skinKinds.find(tr.id);
            if (it != m_skinKinds.end())
                kind = it->second;
            if (kind == kSkinBoxSpin) setSpinValue(tr.id, static_cast<int>(v));
            else if (kind == kSkinBoxDial) setDial(tr.id, static_cast<int>(v));
            else if (kind == kSkinBoxScrollBar) setScrollBarValue(tr.id, static_cast<int>(v));
            else if (kind == kSkinBoxRange) setRangeValues(tr.id, rangeMinOf(tr.id),
                                                           static_cast<int>(v));
            else if (h != nullptr)
            {
                wchar_t cls[64] = {0};
                ::GetClassNameW(h, cls, 64);
                if (_wcsicmp(cls, L"msctls_trackbar32") == 0)
                    ::SendMessageW(h, TBM_SETPOS, TRUE, static_cast<LPARAM>(static_cast<int>(v)));
                else if (_wcsicmp(cls, L"msctls_progress32") == 0)
                    ::SendMessageW(h, PBM_SETPOS, static_cast<WPARAM>(static_cast<int>(v)), 0);
            }
        }
        m_animApplying = false;
    }

double PyMfcHost::currentPropValue(int id, const std::wstring& prop) const
{
        if (prop == L"Value")
        {
            int kind = 0;
            auto it = m_skinKinds.find(id);
            if (it != m_skinKinds.end())
                kind = it->second;
            if (kind == kSkinBoxSpin) return getSpinValue(id);
            if (kind == kSkinBoxDial) return getDial(id);
            if (kind == kSkinBoxScrollBar) return getScrollBarValue(id);
            if (kind == kSkinBoxRange) return getRangeHigh(id);
            if (HWND h = ::GetDlgItem(m_hWnd, id))
            {
                wchar_t cls[64] = {0};
                ::GetClassNameW(h, cls, 64);
                if (_wcsicmp(cls, L"msctls_trackbar32") == 0)
                    return static_cast<double>(::SendMessageW(h, TBM_GETPOS, 0, 0));
                if (_wcsicmp(cls, L"msctls_progress32") == 0)
                    return static_cast<double>(::SendMessageW(h, PBM_GETPOS, 0, 0));
            }
            return 0.0;
        }
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return 0.0;
        RECT rc = {};
        ::GetWindowRect(h, &rc);
        POINT tl = {rc.left, rc.top};
        POINT br = {rc.right, rc.bottom};
        ::MapWindowPoints(HWND_DESKTOP, m_hWnd, &tl, 1);
        ::MapWindowPoints(HWND_DESKTOP, m_hWnd, &br, 1);
        if (prop == L"X") return tl.x;
        if (prop == L"Y") return tl.y;
        if (prop == L"Width") return br.x - tl.x;
        if (prop == L"Height") return br.y - tl.y;
        return 0.0;
    }

void PyMfcHost::animate(int id, const std::wstring& prop, double to, int dur,
                 const std::string& ease, int delay, bool loop)
{
        animateFrom(id, prop, currentPropValue(id, prop), to, dur, ease, delay, loop);
    }

void PyMfcHost::animateFrom(int id, const std::wstring& prop, double from, double to, int dur,
                     const std::string& ease, int delay, bool loop)
{
        AnimTrack tr;
        tr.id = id;
        tr.prop = prop;
        tr.from = from;
        tr.to = to;
        tr.t0 = delay < 0 ? 0 : delay;
        tr.dur = dur < 1 ? 1 : dur;
        tr.ease = ease;
        tr.loop = loop;
        animAdd(tr);
    }

void PyMfcHost::animateColor(int id, COLORREF to, int dur, const std::string& ease, int delay)
{
        AnimTrack tr;
        tr.id = id;
        tr.prop = L"Color";
        tr.color = true;
        tr.c0 = m_ctrlBack.count(id) ? m_ctrlBack[id] : RGB(49, 56, 66);
        tr.c1 = to;
        tr.t0 = delay < 0 ? 0 : delay;
        tr.dur = dur < 1 ? 1 : dur;
        tr.ease = ease;
        animAdd(tr);
    }

void PyMfcHost::animateState(int id, const std::wstring& prop, double to, int dur,
                      const std::string& ease, int delay)
{
        animate(id, prop, to, dur, ease, delay, false);
    }

void PyMfcHost::stopAnimations(int id)
{
        if (id == 0)
            m_anims.clear();
        else
            m_anims.erase(std::remove_if(m_anims.begin(), m_anims.end(),
                                         [id](const AnimTrack& t) { return t.id == id; }),
                          m_anims.end());
        if (m_anims.empty() && m_animTickOn && m_animWnd != nullptr)
        {
            ::KillTimer(m_animWnd, kAnimTimerId);
            m_animTickOn = false;
        }
    }

void PyMfcHost::setBehavior(int id, const std::wstring& prop, int dur, const std::string& ease)
{
        m_behaviors[std::to_wstring(id) + L"|" + prop] = {dur, ease};
    }

bool PyMfcHost::behaviorFor(int id, const std::wstring& prop, int& dur, std::string& ease) const
{
        auto it = m_behaviors.find(std::to_wstring(id) + L"|" + prop);
        if (it == m_behaviors.end())
            return false;
        dur = it->second.first;
        ease = it->second.second;
        return true;
    }

void PyMfcHost::animTick()
{
        const DWORD now = static_cast<DWORD>(::GetTickCount64());
        bool alive = false;
        for (size_t i = 0; i < m_anims.size();)
        {
            AnimTrack& tr = m_anims[i];
            long long el = static_cast<long long>(now) - static_cast<long long>(m_animStartMs) - tr.t0;
            if (el < 0)
            {
                ++i;
                alive = true;
                continue;
            }
            double p = static_cast<double>(el) / static_cast<double>(tr.dur);
            if (p > 1.0)
                p = 1.0;
            if (tr.color)
                applyAnimValue(tr, 0.0, lerpColor(tr.c0, tr.c1, easeAt(tr.ease, p)));
            else
            {
                const double v = tr.from + (tr.to - tr.from) * easeAt(tr.ease, p);
                // 末帧必须无条件落到 tr.to：整数型属性（SpinBox 的 Value）在最后一步
                // 往往只差 0.4 个像素级余量，被下面的去抖门限吃掉后，终点会停在 98 而不是 99。
                const bool final = p >= 1.0;
                const bool changed = final || !tr.hasLast || std::fabs(v - tr.lastV) >= 0.5;
                tr.hasLast = true;
                tr.lastV = v;
                if (changed)   // 数值没动就不必 SetWindowPos / 重画，避免空转闪烁
                    applyAnimValue(tr, final ? tr.to : v, 0);
            }
            if (p >= 1.0)
            {
                if (tr.loop)
                {
                    tr.t0 += tr.dur;   // 下一轮
                    ++i;
                    alive = true;
                    continue;
                }
                m_anims.erase(m_anims.begin() + static_cast<std::ptrdiff_t>(i));
                continue;
            }
            ++i;
            alive = true;
        }
        if (!alive && m_animTickOn && m_animWnd != nullptr)
        {
            ::KillTimer(m_animWnd, kAnimTimerId);
            m_animTickOn = false;
        }
    }

void PyMfcHost::addString(int id, const std::wstring& text)
{
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::SendMessageW(h, isComboBox(h) ? CB_ADDSTRING : LB_ADDSTRING, 0,
                           reinterpret_cast<LPARAM>(text.c_str()));
    }

void PyMfcHost::clearStrings(int id)
{
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::SendMessageW(h, isComboBox(h) ? CB_RESETCONTENT : LB_RESETCONTENT, 0, 0);
    }

int PyMfcHost::countStrings(int id) const
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        return h ? static_cast<int>(::SendMessageW(h, isComboBox(h) ? CB_GETCOUNT : LB_GETCOUNT, 0, 0)) : 0;
    }

std::wstring PyMfcHost::getString(int id, int index) const
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr || index < 0)
            return {};
        const UINT msg = isComboBox(h) ? CB_GETLBTEXT : LB_GETTEXT;
        const int len = static_cast<int>(::SendMessageW(h, isComboBox(h) ? CB_GETLBTEXTLEN : LB_GETTEXTLEN,
                                                        static_cast<WPARAM>(index), 0));
        if (len <= 0)
            return {};
        std::wstring text(static_cast<size_t>(len) + 1, L'\0');
        ::SendMessageW(h, msg, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(text.data()));
        text.resize(static_cast<size_t>(len));
        return text;
    }

void PyMfcHost::deleteString(int id, int index)
{
        if (index < 0)
            return;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::SendMessageW(h, isComboBox(h) ? CB_DELETESTRING : LB_DELETESTRING,
                           static_cast<WPARAM>(index), 0);
    }

int PyMfcHost::getCurSel(int id) const
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        return h ? static_cast<int>(::SendMessageW(h, isComboBox(h) ? CB_GETCURSEL : LB_GETCURSEL, 0, 0)) : -1;
    }

void PyMfcHost::setCurSel(int id, int index)
{
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::SendMessageW(h, isComboBox(h) ? CB_SETCURSEL : LB_SETCURSEL,
                           static_cast<WPARAM>(index), 0);
    }

int PyMfcHost::getCheck(int id) const
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        return h ? static_cast<int>(::SendMessageW(h, BM_GETCHECK, 0, 0)) : 0;
    }

void PyMfcHost::setCheck(int id, bool checked)
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return;
        ::SendMessageW(h, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
        // BM_SETCHECK 只改状态不重绘，界面上的勾选标记不会跟着变
        ::InvalidateRect(h, nullptr, TRUE);
        ::UpdateWindow(h);
    }

void PyMfcHost::setRange(int id, int minValue, int maxValue)
{
        // PyMfcSkinBox 的 RangeSlider 没有系统类，量程记在宿主里
        auto itKind = m_skinKinds.find(id);
        if (itKind != m_skinKinds.end() &&
            (itKind->second == kSkinBoxRange || itKind->second == kSkinBoxDial ||
             itKind->second == kSkinBoxScrollBar))
        {
            m_boxRange[id] = std::make_pair(minValue, maxValue);
            // 量程变了只夹一下已有的两个拇指，不能清掉 —— XAML 属性顺序里
            // LowValue/HighValue 常常写在 Minimum/Maximum 前面
            auto lo = m_rangeLo.find(id);
            if (lo != m_rangeLo.end())
                lo->second = lo->second < minValue ? minValue : (lo->second > maxValue ? maxValue : lo->second);
            auto hi = m_rangeHi.find(id);
            if (hi != m_rangeHi.end())
                hi->second = hi->second < minValue ? minValue : (hi->second > maxValue ? maxValue : hi->second);
            return;
        }
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return;
        if (isProgressBar(h))
        {
            ::SendMessageW(h, PBM_SETRANGE32, static_cast<WPARAM>(minValue),
                           static_cast<LPARAM>(maxValue));
        }
        else if (isTrackBar(h))
        {
            ::SendMessageW(h, TBM_SETRANGE, TRUE,
                           static_cast<LPARAM>(MAKELPARAM(minValue, maxValue)));
        }
    }

void PyMfcHost::setPos(int id, int value)
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return;
        if (isProgressBar(h))
        {
            ::SendMessageW(h, PBM_SETPOS, static_cast<WPARAM>(value), 0);
            // PB/TB 设值默认只更新内部状态，界面不重画：值对了但进度条/滑块看不见
            ::InvalidateRect(h, nullptr, TRUE);
            ::UpdateWindow(h);
        }
        else if (isTrackBar(h))
        {
            ::SendMessageW(h, TBM_SETPOS, TRUE, static_cast<LPARAM>(value));
            ::InvalidateRect(h, nullptr, TRUE);
            ::UpdateWindow(h);
        }
    }

int PyMfcHost::getPos(int id) const
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return 0;
        if (isProgressBar(h))
            return static_cast<int>(::SendMessageW(h, PBM_GETPOS, 0, 0));
        if (isTrackBar(h))
            return static_cast<int>(::SendMessageW(h, TBM_GETPOS, 0, 0));
        return 0;
    }

void PyMfcHost::setReadOnly(int id, bool readOnly)
{
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::SendMessageW(h, EM_SETREADONLY, readOnly ? TRUE : FALSE, 0);
    }

bool PyMfcHost::getReadOnly(int id) const
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        return h != nullptr && (::GetWindowLongPtrW(h, GWL_STYLE) & ES_READONLY) != 0;
    }

void* PyMfcHost::getDlgItemHandle(int id) const
{
        return ::GetDlgItem(m_hWnd, id);
    }

void PyMfcHost::setControlLevel(int id, int level)
{ setLevel(id, level); }


int PyMfcHost::getControlLevel(int id) const
{ return getLevel(id); }


void PyMfcHost::setControlTabIndex(int id, int index)
{ setTabIndex(id, index); }


int PyMfcHost::getControlTabIndex(int id) const
{ return getTabIndex(id); }


void PyMfcHost::setControlTheme(int id, const std::wstring& theme)
{ m_theme[id] = theme; }


std::wstring PyMfcHost::getControlTheme(int id) const
{
        auto it = m_theme.find(id);
        return it == m_theme.end() ? std::wstring() : it->second;
    }

void PyMfcHost::setControlAppearance(int id, const xamlapp::XamlAppearance& ap)
{
        debugLogf(L"[ap] id=%d bg='%ls' fg='%ls' empty=%d", id, ap.backColor.c_str(),
                  ap.foreColor.c_str(), ap.empty() ? 1 : 0);
        m_ap[id] = ap;
        auto it = m_apBrush.find(id);
        if (it != m_apBrush.end())
        {
            if (it->second) ::DeleteObject(it->second);
            m_apBrush.erase(it);
        }
        COLORREF bg = m_bg;
        if (!ap.backColor.empty())
        {
            COLORREF c = 0;
            if (xamlapp::ParseColor(ap.backColor, c))
                bg = xamlapp::Blend(c, m_bg, ap.opacity);
        }
        m_apBrush[id] = ::CreateSolidBrush(bg);
        COLORREF fg = contrastText(bg);
        if (!ap.foreColor.empty())
        {
            COLORREF c = 0;
            if (xamlapp::ParseColor(ap.foreColor, c))
                fg = c;
        }
        if (HWND h = ::GetDlgItem(m_hWnd, id))
        {
            applyNativeThemeColors(h, bg, fg);
            ::InvalidateRect(h, nullptr, TRUE);
        }
    }

void PyMfcHost::applyNativeThemeColors(HWND h, COLORREF bg, COLORREF fg)
{
        if (h == nullptr)
            return;
        wchar_t cls[64] = {};
        ::GetClassNameW(h, cls, 64);
        if (_wcsicmp(cls, L"SysListView32") == 0)
        {
            ::SendMessageW(h, LVM_SETBKCOLOR, 0, static_cast<LPARAM>(bg));
            ::SendMessageW(h, LVM_SETTEXTCOLOR, 0, static_cast<LPARAM>(fg));
            ::SendMessageW(h, LVM_SETTEXTBKCOLOR, 0, static_cast<LPARAM>(bg));
        }
        else if (_wcsicmp(cls, L"SysTreeView32") == 0)
        {
            ::SendMessageW(h, TVM_SETBKCOLOR, 0, static_cast<LPARAM>(bg));
            ::SendMessageW(h, TVM_SETTEXTCOLOR, 0, static_cast<LPARAM>(fg));
        }
        else if (_wcsicmp(cls, L"Edit") == 0 || _wcsicmp(cls, L"RICHEDIT50W") == 0)
        {
            // EM_SETBKGNDCOLOR：主题化 Edit 只认这条，WM_CTLCOLOR 的刷子会被忽略
            ::SendMessageW(h, EM_SETBKGNDCOLOR, 0, static_cast<LPARAM>(bg));
        }
        else if (_wcsicmp(cls, L"SysMonthCal32") == 0)
        {
            ::SendMessageW(h, MCM_SETCOLOR, MCSC_BACKGROUND, static_cast<LPARAM>(bg));
            ::SendMessageW(h, MCM_SETCOLOR, MCSC_MONTHBK, static_cast<LPARAM>(bg));
            ::SendMessageW(h, MCM_SETCOLOR, MCSC_TEXT, static_cast<LPARAM>(fg));
            ::SendMessageW(h, MCM_SETCOLOR, MCSC_TITLEBK, static_cast<LPARAM>(m_btn));
            ::SendMessageW(h, MCM_SETCOLOR, MCSC_TITLETEXT, static_cast<LPARAM>(fg));
            ::SendMessageW(h, MCM_SETCOLOR, MCSC_TRAILINGTEXT, static_cast<LPARAM>(fg));
        }
        else if (_wcsicmp(cls, L"msctls_progress32") == 0)
        {
            ::SendMessageW(h, PBM_SETBKCOLOR, 0, static_cast<LPARAM>(bg));
            ::SendMessageW(h, PBM_SETBARCOLOR, 0, static_cast<LPARAM>(RGB(0, 120, 212)));
        }
    }

void PyMfcHost::setControlIcon(int id, const std::wstring& path)
{
        if (path.empty())
        {
            m_icons.erase(id);
            return;
        }
        m_icons[id] = path;
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

const std::wstring* PyMfcHost::findIcon(int id) const
{
        auto it = m_icons.find(id);
        return it == m_icons.end() ? nullptr : &it->second;
    }

const xamlapp::XamlAppearance* PyMfcHost::findAppearance(int id) const
{
        auto it = m_ap.find(id);
        return it == m_ap.end() ? nullptr : &it->second;
    }

COLORREF PyMfcHost::contrastText(COLORREF bg)
{
        const int lum = (GetRValue(bg) * 299 + GetGValue(bg) * 587 + GetBValue(bg) * 114) / 1000;
        return lum > 140 ? RGB(32, 36, 42) : RGB(235, 235, 235);
    }

void PyMfcHost::setPageBackdrop(COLORREF c, bool valid)
{
        m_pageBg = c;
        m_hasPageBg = valid;
        if (m_hasPageBg)
            m_bg = c;
        Invalidate(FALSE);
    }

void PyMfcHost::addContainerBackdrop(int x, int y, int w, int h,
                              const xamlapp::XamlAppearance& ap)
{
        if (ap.empty())
            return;
        m_backdrops.emplace_back(CRect(x, y, x + w, y + h), ap);
    }

void PyMfcHost::clearContainerBackdrops()
{
        m_backdrops.clear();
    }

COLORREF PyMfcHost::surfaceColorAt(const CRect& rc) const
{
        const COLORREF pageBack = m_hasPageBg ? m_pageBg : m_bg;
        const CPoint pt((rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2);
        long bestArea = 0x7FFFFFFF;
        COLORREF best = pageBack;
        for (const auto& bd : m_backdrops)
        {
            if (!bd.first.PtInRect(pt))
                continue;
            const long area = static_cast<long>(bd.first.Width()) * bd.first.Height();
            if (area >= bestArea)
                continue;
            COLORREF c = 0;
            if (!xamlapp::ParseColor(bd.second.backColor, c))
                continue;
            best = xamlapp::Blend(c, pageBack, bd.second.opacity);
            bestArea = area;
        }
        return best;
    }

void PyMfcHost::setFocusTo(int id)
{
        if (HWND h = ::GetDlgItem(m_hWnd, id))
            ::SetFocus(h);
    }

bool PyMfcHost::getVisible(int id) const
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        return h != nullptr && ::IsWindowVisible(h) != FALSE;
    }

bool PyMfcHost::getEnabled(int id) const
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        return h != nullptr && ::IsWindowEnabled(h) != FALSE;
    }

void PyMfcHost::onClick(int id, py::object callback)
{ m_click[id] = std::move(callback); }

void PyMfcHost::onControlEvent(int id, const std::wstring& ev, py::object callback)
{
    m_ctlEvents[std::to_wstring(id) + L":" + ev] = std::move(callback);
    m_anyCtlEvent = true;
    // 已经建好的控件要补挂子类化（事件登记晚于控件创建是常态）
    HWND h = ::GetDlgItem(m_hWnd, id);
    if (h == nullptr)
    {
        // 控件不一定在顶层窗口直接子级，枚举整棵树找
        struct Ctx { int id; HWND out; } ctx = { id, nullptr };
        ::EnumChildWindows(m_hWnd, [](HWND c, LPARAM p) -> BOOL {
            Ctx* c2 = reinterpret_cast<Ctx*>(p);
            if (static_cast<int>(::GetDlgCtrlID(c)) == c2->id) { c2->out = c; return FALSE; }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&ctx));
        h = ctx.out;
    }
    if (h != nullptr && ::GetPropW(h, kEvtHostProp) == nullptr)
    {
        WNDPROC cur = reinterpret_cast<WNDPROC>(
            ::SetWindowLongPtrW(h, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&PyMfcEventSubProc)));
        ::SetPropW(h, kEvtPrevProcProp, reinterpret_cast<HANDLE>(cur));
        ::SetPropW(h, kEvtHostProp, reinterpret_cast<HANDLE>(this));
    }
}

void PyMfcHost::fireControlEvent(int id, const std::wstring& ev)
{
    if (m_ctlEvents.empty())
        return;
    auto it = m_ctlEvents.find(std::to_wstring(id) + L":" + ev);
    if (it == m_ctlEvents.end())
        return;
    invokePython(it->second);
}


void PyMfcHost::onGridAction(py::object cb)
{ m_gridAction = std::move(cb); }


void PyMfcHost::fireGridAction(int ctlId, const char* action, int row, int col, const std::wstring& text)
{
        py::object fn;
        auto it = m_gridActionFor.find(ctlId);
        if (it != m_gridActionFor.end())
            fn = it->second;            // 先给按控件登记的回调，避免多表串台
        if (!fn && m_gridAction)
            fn = m_gridAction;          // 没按控件登记才退回全局
        if (!fn)
            return;
        py::gil_scoped_acquire gil;
        try
        {
            fn(std::string(action), row, col, text);
        }
        catch (const py::error_already_set& e)
        {
            debugLogf(L"[grid] 回调异常: %hs", e.what());
        }
    }

void PyMfcHost::onGridActionFor(int id, py::object callback)
{ m_gridActionFor[id] = std::move(callback); }

std::wstring PyMfcHost::getXamlValue(int id, const std::wstring& prop) const
{
        const std::wstring k = prop;
        if (k == L"text")
            return getText(id);
        if (k == L"checked")
            return getCheck(id) != 0 ? L"true" : L"false";
        if (k == L"index" || k == L"curSel")
            return std::to_wstring(getCurSel(id));
        if (k == L"value")
            return std::to_wstring(getPos(id));
        if (k == L"visible")
            return getVisible(id) ? L"true" : L"false";
        if (k == L"enabled")
            return getEnabled(id) ? L"true" : L"false";
        if (k == L"readOnly")
            return getReadOnly(id) ? L"true" : L"false";
        if (k == L"rowCount")
            return std::to_wstring(getTableRowCount(id));
        auto it = m_ap.find(id);
        if (it != m_ap.end())
            return xamlapp::GetProperty(it->second, k);   // back/fore/border/opacity/font 等外观
        return std::wstring();
    }

void PyMfcHost::onDrag(int id, py::object cb)
{ m_drag[id] = std::move(cb); }


void PyMfcHost::fireDrag(int id, const char* phase, int x)
{
        auto it = m_drag.find(id);
        if (it == m_drag.end())
            return;
        py::gil_scoped_acquire gil;
        try { it->second(std::string(phase), x); }
        catch (const py::error_already_set& e) { debugLogf(L"[drag] %hs", e.what()); }
    }

void PyMfcHost::onBlur(int id, py::object cb)
{ m_blur[id] = std::move(cb); }


void PyMfcHost::fireBlur(int id)
{
        auto it = m_blur.find(id);
        if (it == m_blur.end())
            return;
        py::gil_scoped_acquire gil;
        try { it->second(); }
        catch (const py::error_already_set& e) { debugLogf(L"[blur] %hs", e.what()); }
    }

HWND PyMfcHost::findByIdDeep(HWND parent, int id)
{
        HWND direct = ::GetDlgItem(parent, id);
        if (direct != nullptr && ::IsWindow(direct))
            return direct;
        for (HWND c = ::GetWindow(parent, GW_CHILD); c != nullptr;
             c = ::GetWindow(c, GW_HWNDNEXT))
        {
            if (HWND hit = findByIdDeep(c, id))
                return hit;
        }
        return nullptr;
    }

py::object PyMfcHost::cellRect(int id, int row, int col)
{
        if (TableControl* nt = findNewTable(id)) return tableCellRect(id, row, col);
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return py::none();
        wchar_t cls[64] = {};
        ::GetClassNameW(h, cls, 64);
        if (_wcsicmp(cls, L"SysListView32") != 0)
            return py::none();
        // LVM_GETSUBITEMRECT 在本机 SysListView32 上水平方向恒返回 0 宽（left==right），
        // 拿不到单元格左右边界。改成"行矩形 + 累加列宽"自己算，实测稳定。
        RECT rowRc = { 0, 0, 0, 0 };
        if (::SendMessageW(h, LVM_GETITEMRECT, static_cast<WPARAM>(row),
                           reinterpret_cast<LPARAM>(&rowRc)) == FALSE)
            return py::none();
        int colX = 0, colW = 0;
        for (int i = 0; i <= col; ++i)
        {
            const int cw = static_cast<int>(::SendMessageW(h, LVM_GETCOLUMNWIDTH,
                                                           static_cast<WPARAM>(i), 0));
            if (cw <= 0)
                return py::none();
            if (i < col)
                colX += cw;
            else
                colW = cw;
        }
        POINT org = { 0, 0 };
        ::SendMessageW(h, LVM_GETORIGIN, 0, reinterpret_cast<LPARAM>(&org));
        const int x = colX - org.x;         // 横向滚动补偿（表格无横滚时 org.x=0）
        const int hgt = rowRc.bottom - rowRc.top;
        debugLogf(L"[cellrect] id=%d row=%d col=%d -> (%d,%d,%d,%d)",
                  id, row, col, x, rowRc.top, colW, hgt);
        if (colW <= 0 || hgt <= 0)
            return py::none();
        return py::make_tuple(x, rowRc.top, colW, hgt);
    }

bool PyMfcHost::installDragBar(int id)
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return false;
        WNDPROC old = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(
            h, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&PyMfcDragBarProc)));
        if (old == nullptr)
            return false;
        if (old == &PyMfcDragBarProc)                   // 重复安装：否则会自己转发给自己（栈溢出）
            return true;
        skinOldProcs()[h] = old;
        ::SetPropW(h, kSkinHostProp, reinterpret_cast<HANDLE>(this));
        return true;
    }

void PyMfcHost::setWheelRegion(int x, int y, int w, int h, int step)
{
        m_wheelOn = (w > 0 && h > 0);
        m_wheelRect.left = x; m_wheelRect.top = y;
        m_wheelRect.right = x + w; m_wheelRect.bottom = y + h;
        m_wheelStep = step > 0 ? step : 1;
    }

void PyMfcHost::onWheel(py::object cb)
{ m_wheelCb = std::move(cb); }


bool PyMfcHost::fireWheelMessage(WPARAM wp, LPARAM lp)
{
        if (!m_wheelOn) return false;
        POINT pt = {};
        pt.x = GET_X_LPARAM(lp);
        pt.y = GET_Y_LPARAM(lp);
        ::ScreenToClient(m_hWnd, &pt);
        if (pt.x < m_wheelRect.left || pt.x > m_wheelRect.right ||
            pt.y < m_wheelRect.top || pt.y > m_wheelRect.bottom)
            return false;
        const int dir = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? -1 : 1;   // 往上滚 = 行号减小
        py::gil_scoped_acquire gil;
        if (!m_wheelCb || m_wheelCb.is_none())
            return false;
        try
        {
            m_wheelCb(dir * m_wheelStep);
        }
        catch (py::error_already_set& e)
        {
            debugLogf(L"[wheel] 回调异常: %hs", e.what());
        }
        return true;
    }

void PyMfcHost::setUrl(int id, const std::wstring& url)
{
        m_urls[id] = url;
        applyLinkText(id);
    }

std::wstring PyMfcHost::getUrl(int id) const
{
        auto it = m_urls.find(id);
        if (it != m_urls.end())
            return it->second;
        // 没有记录时从控件文本里解析，兼容 XAML 直接写成 <a href="..."> 的情况
        const std::wstring text = getText(id);
        const size_t a = text.find(L"href=\"");
        if (a == std::wstring::npos)
            return {};
        const size_t b = text.find(L"\"", a + 6);
        return b == std::wstring::npos ? std::wstring() : text.substr(a + 6, b - a - 6);
    }

bool PyMfcHost::setImage(int id, const std::wstring& path)
{
        m_images[id] = path;
        // 路径解析顺序：原样（绝对/相对当前目录）→ pyd 所在目录（与 Icon="res/x.png" 一致）。
        // 只按后一种解析会让 XAML 里写的 res/xxx.png 找不到文件（XAML 旁边才有 res/）。
        std::wstring full = path;
        if (!full.empty() && !(full.size() > 2 && full[1] == L':') &&
            full[0] != L'\\' && full[0] != L'/')
        {
            if (::GetFileAttributesW(full.c_str()) == INVALID_FILE_ATTRIBUTES)
            {
                const std::wstring alt = pymfcDir() + L"\\" + full;
                if (::GetFileAttributesW(alt.c_str()) != INVALID_FILE_ATTRIBUTES)
                    full = alt;
            }
        }
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return false;
        HBITMAP bmp = full.empty() ? nullptr : loadBitmapFromFile(full);
        wchar_t cls[64] = {};
        ::GetClassNameW(h, cls, 64);
        if (_wcsicmp(cls, L"Static") == 0)
            ::SendMessageW(h, STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(bmp));
        else if (_wcsicmp(cls, L"Button") == 0)
        {
            if (bmp != nullptr)
                ::SetWindowLongPtrW(h, GWL_STYLE,
                                    ::GetWindowLongPtrW(h, GWL_STYLE) | BS_BITMAP);
            ::SendMessageW(h, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(bmp));
        }
        // 自绘画布那条通路也要知道这张图，否则 owner-draw 的外观里仍然没图
        setControlIcon(id, full);
        if (bmp != nullptr)
        {
            auto it = m_ownedBitmaps.find(id);
            if (it != m_ownedBitmaps.end() && it->second != nullptr && it->second != bmp)
                ::DeleteObject(it->second);
            m_ownedBitmaps[id] = bmp;
        }
        ::InvalidateRect(h, nullptr, TRUE);
        ::UpdateWindow(h);
        return bmp != nullptr;
    }

void PyMfcHost::setRichFormat(int id, int sizePt, bool bold, bool italic, unsigned long rgb)
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return;
        CHARFORMAT2W cf = {};
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_BOLD | CFM_ITALIC | CFM_SIZE | CFM_COLOR;
        cf.dwEffects = (bold ? CFE_BOLD : 0) | (italic ? CFE_ITALIC : 0);
        cf.yHeight = (sizePt > 0 ? sizePt : 9) * 20;   // 1pt = 20 twips
        cf.crTextColor = RGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
        ::SendMessageW(h, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&cf));
        ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::setScrollRange(int id, int minValue, int maxValue)
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return;
        m_scroll[id] = std::make_pair(minValue, maxValue);
        if (m_scrollPos.find(id) == m_scrollPos.end())
            m_scrollPos[id] = minValue;      // 初始位置 = 最小值
        // 自绘滚动区域不能调 SetScrollInfo：系统会给窗口补上一条原生滚动条（WS_VSCROLL），
        // 抢走 17px 客户区并在右侧留下一条白条。
        if (m_scrollAreas.count(id) != 0)
        {
            ::InvalidateRect(h, nullptr, FALSE);
            return;
        }
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE;
        si.nMin = minValue;
        si.nMax = maxValue;
        si.nPage = 10;
        ::SetScrollInfo(h, SB_VERT, &si, TRUE);
        ::InvalidateRect(h, nullptr, TRUE);
    }

std::pair<int, int> PyMfcHost::getScrollRange(int id) const
{
        auto it = m_scroll.find(id);
        if (it != m_scroll.end())
            return std::make_pair(it->second.first, it->second.second);
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return {0, 0};
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE;
        ::GetScrollInfo(h, SB_VERT, &si);
        return {si.nMin, si.nMax};
    }

std::pair<int, int> PyMfcHost::getSelection(int id) const
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return { 0, 0 };
        wchar_t cls[64] = {};
        ::GetClassNameW(h, cls, 64);
        if (_wcsicmp(cls, L"RICHEDIT50W") == 0)
        {
            CHARRANGE cr = {};
            ::SendMessageW(h, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&cr));
            return { static_cast<int>(cr.cpMin), static_cast<int>(cr.cpMax) };
        }
        DWORD s = 0;
        DWORD e = 0;
        ::SendMessageW(h, EM_GETSEL, reinterpret_cast<WPARAM>(&s), reinterpret_cast<LPARAM>(&e));
        return { static_cast<int>(s), static_cast<int>(e) };
    }

void PyMfcHost::setSelection(int id, int from, int to) const
{
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return;
        if (to < from)
            to = from;
        ::SendMessageW(h, EM_SETSEL, static_cast<WPARAM>(from), static_cast<LPARAM>(to));
        if (::GetFocus() == h)
            ::SendMessageW(h, EM_SCROLLCARET, 0, 0);
        ::InvalidateRect(h, nullptr, FALSE);
    }

int PyMfcHost::getScrollPos(int id) const
{        auto it = m_scrollPos.find(id);
        if (it != m_scrollPos.end())
            return it->second;
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return 0;
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS;
        ::GetScrollInfo(h, SB_VERT, &si);
        return si.nPos;
    }

void PyMfcHost::onChange(int id, py::object callback)
{ m_change[id] = std::move(callback); }


void PyMfcHost::onSelect(int id, py::object callback)
{ m_select[id] = std::move(callback); }


void PyMfcHost::onDoubleClick(int id, py::object callback)
{ m_dblclick[id] = std::move(callback); }


void PyMfcHost::onDropDown(int id, py::object callback)
{ m_dropdown[id] = std::move(callback); }


void PyMfcHost::onValue(int id, py::object callback)
{ m_value[id] = std::move(callback); }


void PyMfcHost::onChecked(int id, py::object callback)
{ m_checked[id] = std::move(callback); }


void PyMfcHost::clearCallbacks()
{
    m_ctlEvents.clear();
        m_click.clear();
        m_change.clear();
        m_select.clear();
        m_dblclick.clear();
        m_dropdown.clear();
        m_value.clear();
        m_checked.clear();
    }
