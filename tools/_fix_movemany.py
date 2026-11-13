import io
p = 'G:/PyArx/PyMfcXaml/PyMfcHost.cpp'
s = io.open(p, encoding='utf-8-sig').read()

old = """        std::vector<HWND> moved;
        moved.reserve(static_cast<size_t>(n));"""
new = """        std::vector<HWND> moved;
        moved.reserve(static_cast<size_t>(n));
        std::vector<RECT> oldRects;              // 每个控件挪动前的父客户区矩形
        oldRects.reserve(static_cast<size_t>(n));"""
assert s.count(old) == 1
s = s.replace(old, new)

old2 = """            dwp = ::DeferWindowPos(dwp, c, nullptr, x, y, w, hh, flags);
            if (dwp == nullptr)
                return;
            moved.push_back(c);"""
new2 = """            RECT before = { 0, 0, 0, 0 };
            ::GetWindowRect(c, &before);
            ::MapWindowPoints(HWND_DESKTOP, m_hWnd, reinterpret_cast<LPPOINT>(&before), 2);
            dwp = ::DeferWindowPos(dwp, c, nullptr, x, y, w, hh, flags);
            if (dwp == nullptr)
                return;
            moved.push_back(c);
            oldRects.push_back(before);"""
assert s.count(old2) == 1
s = s.replace(old2, new2)

old3 = """        // 异步标脏：让系统在消息循环里合并成一次重画，同步 UPDATE 反而更容易看到白闪
        ::RedrawWindow(m_hWnd, nullptr, nullptr, RDW_INVALIDATE);"""
new3 = """        // 父窗口只标"动过的并集矩形"，不再整窗 RDW_INVALIDATE：
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
            ::InvalidateRect(m_hWnd, &uni, FALSE);"""
assert s.count(old3) == 1
s = s.replace(old3, new3)
io.open(p, 'w', encoding='utf-8-sig', newline='').write(s)
print('moveMany：父窗口只标并集矩形')
