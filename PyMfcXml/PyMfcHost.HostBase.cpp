// PyMfcHost.HostBase.cpp —— 拆分自 PyMfcHost.cpp（按职责分组；接口声明仍在 PyMfcHost.h）。
#include "PyMfcHostInternal.h"

void PyMfcHost::PostNcDestroy()
{}

HWND PyMfcHost::hostHwnd() const
{
    return GetSafeHwnd();
}

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
        clearIdCache();   // 新窗口，旧句柄一律作废
        ::SetPropW(hwnd, kTabHostProp, reinterpret_cast<HANDLE>(1));
        ensureTabHook();
        setTheme(true);
        ensureSurface();
        return TRUE;
    }

void PyMfcHost::ensureSurface()
{
    if (m_surface != nullptr) return;

    // 画布后端：默认 D3D11（D3D11 设备 + DXGI 交换链 + D2D 目标；硬件优先，失败退 WARP，
    // 再失败就静静留在 GDI，老界面行为不变）。
    //   PYARX_SURFACE=0 → 强制旧 GDI 通道（排障/像素对比用）
    //   PYARX_SURFACE=1 → 与缺省一致（老脚本写法不用改）
    wchar_t v[4] = {};
    if (::GetEnvironmentVariableW(L"PYARX_SURFACE", v, 4) >= 1 && v[0] == L'0')
        return;

    CRect rc;
    GetClientRect(&rc);
    IXmlSurface* surface = XmlSurfaceCreateD3D11();
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
        // 第六批：自建画布窗口（Switch / SpinBox / RangeSlider / Busy / PageDots，HTML 对齐）
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

void PyMfcHost::ensureWicCom()
{
    static bool once = false;
    if (once)
        return;
    once = true;
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
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

void* PyMfcHost::getDlgItemHandle(int id) const
{
        // 真父子层级下控件可能挂在容器窗口里（第三层起就不是主窗的直接子），
        // ::GetDlgItem 只认直接子，会返回空 —— 裁掉了"子控件的子控件"的父句柄，
        // 表现为三层嵌套时最内层又退回父窗口（扁平兄弟）。这里统一走深度查找。
        return findByIdDeep(m_hWnd, id);
    }

void* PyMfcHost::getHostWindow() const
{
    return m_hWnd;
}

std::pair<int, int> PyMfcHost::controlClientSize(int id) const
{
    HWND hwnd = findByIdDeep(m_hWnd, id);
    if (hwnd == nullptr)
        return {0, 0};
    CRect rc;
    ::GetClientRect(hwnd, &rc);
    return {rc.Width(), rc.Height()};
}

void PyMfcHost::getControlRect(int id, int& x, int& y, int& w, int& h) const
{
    x = 0; y = 0; w = 0; h = 0;
    HWND hwnd = findByIdDeep(m_hWnd, id);
    if (hwnd == nullptr)
        return;
    // 取相对宿主的窗口矩形（含边框），容器排子项用的是"控件占位"，与 moveControl 同一坐标系
    const CRect rc = hostRectOfWindow(hwnd);
    x = rc.left;
    y = rc.top;
    w = rc.Width();
    h = rc.Height();
}

CRect PyMfcHost::hostRectOfWindow(HWND hw) const
{
    CRect rc(0, 0, 0, 0);
    if (hw == nullptr || m_hWnd == nullptr || !::IsWindow(hw))
        return rc;
    ::GetWindowRect(hw, &rc);
    // ScreenToClient 直接对宿主做：中间隔了几层容器都无所谓，不需要逐层累加。
    ::ScreenToClient(m_hWnd, &rc.TopLeft());
    ::ScreenToClient(m_hWnd, &rc.BottomRight());
    return rc;
}

void PyMfcHost::setFocusTo(int id)
{
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::SetFocus(h);
    }

void PyMfcHost::clearIdCache()
{
        idCacheStore().clear();
    }

void PyMfcHost::registerChildId(HWND hostRoot, int id, HWND child)
{
        if (hostRoot == nullptr || child == nullptr) return;
        idCacheStore()[id] = std::make_pair(hostRoot, child);
    }

HWND PyMfcHost::findByIdTree(HWND parent, int id)
{
        HWND direct = ::GetDlgItem(parent, id);
        if (direct != nullptr && ::IsWindow(direct))
            return direct;
        for (HWND c = ::GetWindow(parent, GW_CHILD); c != nullptr;
             c = ::GetWindow(c, GW_HWNDNEXT))
        {
            if (HWND hit = findByIdTree(c, id))
                return hit;
        }
        return nullptr;
    }

HWND PyMfcHost::findByIdDeep(HWND parent, int id)
{
        if (parent == nullptr) return nullptr;
        const HWND root = ::GetAncestor(parent, GA_ROOT);
        auto it = idCacheStore().find(id);
        if (it != idCacheStore().end())
        {
            HWND h = it->second.second;
            // 三重校验：活着 / 控件 id 对得上 / 在同一棵窗口树里。
            // 挡的是句柄回收（重建后同一数值被别的控件占用）与多宿主同 id。
            if (it->second.first == root && ::IsWindow(h) && ::GetDlgCtrlID(h) == id)
                return h;
            idCacheStore().erase(it);
        }
        HWND h = findByIdTree(parent, id);
        if (h != nullptr)
            idCacheStore()[id] = std::make_pair(root, h);
        return h;
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
