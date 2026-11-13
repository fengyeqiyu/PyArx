import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

anchor = """    if (msg == WM_PAINT)
    {
        PAINTSTRUCT ps = {};
        HDC hdc = ::BeginPaint(hwnd, &ps);
        HWND parent = ::GetParent(hwnd);"""
PRINTCLIENT = """    if (msg == WM_PRINTCLIENT)
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
"""
assert t.count(anchor) == 1
t = t.replace(anchor, PRINTCLIENT + anchor)

old2 = """static constexpr const wchar_t* kGridHostProp = L"PyMfcGridHost";"""
new2 = """static bool wicWritePng(const std::wstring& path, int w, int h, void* bgra);
static constexpr const wchar_t* kGridHostProp = L"PyMfcGridHost";"""
assert t.count(old2) == 1
t = t.replace(old2, new2)

old3 = """        for (const xamlplan::PowerBox& b : st.plan.boxes)
        {
            if (mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h)
                return b.code;
        }
        return {};
    }"""
new3 = """        for (const xamlplan::PowerBox& b : st.plan.boxes)
        {
            if (mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h)
                return b.code;
        }
        // 连线命中：贝塞尔折线到光标的最近距离（容差按屏幕 8px 折算成模型单位）。
        // 点中一条线 = 选中它的下游节点：卡片描边 + 父链边高亮 + 详情一起对上。
        const float tol = 8.0f / (st.zoom > 0.01f ? st.zoom : 0.01f);
        for (const xamlplan::PowerLink& l : st.plan.links)
        {
            std::vector<xamlgeom::Point2> pts;
            xamlgeom::bezierPoints(xamlgeom::Point2(l.bez[0], l.bez[1]),
                                   xamlgeom::Point2(l.bez[2], l.bez[3]),
                                   xamlgeom::Point2(l.bez[4], l.bez[5]),
                                   xamlgeom::Point2(l.bez[6], l.bez[7]), 24, pts);
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
    }"""
assert t.count(old3) == 1
t = t.replace(old3, new3)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('A/B/C 已改')
