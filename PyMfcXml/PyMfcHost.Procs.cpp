// PyMfcHost.Procs.cpp —— 拆分自 PyMfcHost.cpp（按职责分组；接口声明仍在 PyMfcHost.h）。
#include "PyMfcHostInternal.h"

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

    // 绘图画布（DWG 为主，兼容已算坐标的布局输入）先接管鼠标：它要自己处理滚轮缩放与拖动平移，不能再转给 Static 的类过程
    if (msg == WM_MOUSEWHEEL || msg == WM_LBUTTONDOWN || msg == WM_MOUSEMOVE ||
        msg == WM_LBUTTONUP || msg == WM_LBUTTONDBLCLK || msg == WM_MBUTTONDOWN ||
        msg == WM_MBUTTONUP || msg == WM_SETCURSOR ||
        msg == WM_MOUSELEAVE || msg == WM_CAPTURECHANGED)
    {
        HANDLE prop = ::GetPropW(hwnd, kDwgHostProp);
        if (prop != nullptr)
        {
            PyMfcHost* host = reinterpret_cast<PyMfcHost*>(prop);
            const int cid = ::GetDlgCtrlID(hwnd);
            debugLogf(L"[mouse-dispatch] cid=%d msg=0x%04X", cid, msg);
            // FileTab：命中/选中/关闭/新建全部走控件库 xmlctl，先于 Ribbon 与绘图画布接管
            if (host->filetabHas(cid))
            {
                const int rx = GET_X_LPARAM(lParam);
                const int ry = GET_Y_LPARAM(lParam);
                host->fileTabMouse(cid, msg, rx, ry, static_cast<WPARAM>(wParam));
                return 0;
            }
            // Ribbon：命中/选中/命令全部走控件库 xmlctl，先于绘图画布（DWG 为主，兼容已算坐标的布局输入）接管
            if (host->ribbonHas(cid))
            {
                const int rx = GET_X_LPARAM(lParam);
                const int ry = GET_Y_LPARAM(lParam);
                host->ribbonMouse(cid, msg, rx, ry, static_cast<WPARAM>(wParam));
                return 0;
            }
            bool handled = false;
            const LRESULT r = PyMfcDwgCanvasMouse(host, hwnd, msg, wParam, lParam, handled);
            if (handled)
                return r;
        }
        // 三维画布：左键拖动旋转视口、滚轮缩放、中键平移、双击复位
        HANDLE p3 = ::GetPropW(hwnd, kView3DHostProp);
        if (p3 != nullptr)
        {
            bool handled = false;
            const LRESULT r = PyMfcView3DMouse(
                reinterpret_cast<PyMfcHost*>(p3), hwnd, msg, wParam, lParam, handled);
            if (handled)
                return r;
        }
    }

    // CSS overflow:scroll/auto：滚轮落在可滚容器内的任一子控件上，就把容器内容整体滚动。
    // 先于 hover 处理，滚完直接吃掉消息，避免控件自己再滚一遍。
    if (msg == WM_MOUSEWHEEL)
    {
        PyMfcHost* host = reinterpret_cast<PyMfcHost*>(::GetPropW(hwnd, kSkinHostProp));
        if (host != nullptr && !host->m_scrollContainers.empty())
        {
            const int notches = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            if (host->scrollOverflowFrom(hwnd, notches))
                return 0;
        }
    }

    // CSS :hover：这个过程一定挂在 owner-draw 控件上（Button/Static/卡的 *_OWNERDRAW），
    // 比"事件子类化"那条链更可靠。进入/离开只在状态翻转时重绘一次，
    // WM_MOUSEMOVE 每秒几十次也不会持续刷帧。
    if (msg == WM_MOUSEMOVE)
    {
        if (PyMfcHoverMark(hwnd))
        {
            if (PyMfcHost* host = reinterpret_cast<PyMfcHost*>(::GetPropW(hwnd, kSkinHostProp)))
                host->startHoverTransition(::GetDlgCtrlID(hwnd), true);
            ::InvalidateRect(hwnd, nullptr, FALSE);
        }
    }
    else if (msg == WM_MOUSELEAVE)
    {
        if (PyMfcHoverUnmark(hwnd))
        {
            if (PyMfcHost* host = reinterpret_cast<PyMfcHost*>(::GetPropW(hwnd, kSkinHostProp)))
                host->startHoverTransition(::GetDlgCtrlID(hwnd), false);
            ::InvalidateRect(hwnd, nullptr, FALSE);
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
        HANDLE szProp = ::GetPropW(hwnd, kDwgHostProp);
        if (szProp != nullptr)
        {
            // 仅 plan 卡片画布(mode1)随尺寸自适应；SVG 图纸场景(mode2)保留用户平移/缩放视图，
            // 不能在布局重排时把 ox/oy/zoom 清零，否则拖完/缩放后视图立马跳回原位。
            PyMfcHost* h = reinterpret_cast<PyMfcHost*>(szProp);
            const int cid = ::GetDlgCtrlID(hwnd);
            auto it = h->m_dwgCv.find(cid);
            if (it != h->m_dwgCv.end() && it->second.mode == 1)
                h->fitDwgCanvas(cid);
        }
        HANDLE sz3 = ::GetPropW(hwnd, kView3DHostProp);
        if (sz3 != nullptr)
        {
            PyMfcHost* host = reinterpret_cast<PyMfcHost*>(sz3);
            auto it = host->m_view3D.find(::GetDlgCtrlID(hwnd));
            if (it != host->m_view3D.end() && it->second.autoFit && !it->second.empty)
                host->view3DFit(::GetDlgCtrlID(hwnd));
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
        // 绘图画布（DWG 为主，兼容已算坐标的布局输入）整块自己铺底（drawDwgCanvas 第一件事就是 fillRect 满客户区），
        // 拖动/缩放时每帧先擦一遍父背景再画，屏幕上就是整块画布白闪一下。
        // 画布自己会覆盖全部像素，所以这里声明"已擦除"即可，不填任何东西。
        if (::GetPropW(hwnd, kDwgHostProp) != nullptr)
            return 1;
        if (::GetPropW(hwnd, kView3DHostProp) != nullptr)
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
