import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

a = '''    void moveControl(int id, int x, int y, int w, int h)
    {
        if (HWND hwnd = ::GetDlgItem(m_hWnd, id))
            ::MoveWindow(hwnd, x, y, w, h, TRUE);
    }
'''
assert t.count(a) == 1, 'A'
A = a + '''
    // 批量布局：一次 DeferWindowPos 提交所有行控件的位移 + 显隐。
    // 逐条 moveControl / ShowWindow 时，每次调用都会让父窗口补画一次露出来的空白，
    // 312 个控件滚一下就是一片闪烁；这里用 SWP_NOREDRAW 攒成一次提交，再统一重画。
    void moveMany(py::list items)
    {
        const int n = static_cast<int>(items.size());
        if (n <= 0 || m_hWnd == nullptr)
            return;
        HDWP dwp = ::BeginDeferWindowPos(n);
        if (dwp == nullptr)
            return;
        std::vector<HWND> moved;
        moved.reserve(static_cast<size_t>(n));
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
            UINT flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW;
            flags |= vis ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;
            dwp = ::DeferWindowPos(dwp, c, nullptr, x, y, w, hh, flags);
            if (dwp == nullptr)
                return;
            moved.push_back(c);
        }
        ::EndDeferWindowPos(dwp);
        // 只把"动过的子窗口"和父窗口露出的空白标脏。不用 RDW_ALLCHILDREN，
        // 否则会把 D2D 画布也拉进来整张重画，滚动一下子就卡。
        for (HWND c : moved)
            ::InvalidateRect(c, nullptr, TRUE);
        ::RedrawWindow(m_hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
'''
t = t.replace(a, A)

d1 = "    void moveControl(int id, int x, int y, int w, int h) { m_host.moveControl(id, x, y, w, h); }\n"
assert t.count(d1) == 1, 'D1'
t = t.replace(d1, d1 + "    void moveMany(py::list items) { m_host.moveMany(std::move(items)); }\n")
d2 = "    void move(int id, int x, int y, int w, int h) { m_host->moveControl(id, x, y, w, h); }\n"
assert t.count(d2) == 1, 'D2'
t = t.replace(d2, d2 + "    void moveMany(py::list items) { m_host->moveMany(std::move(items)); }\n")

e1 = '        .def("move", &PyMfcWindow::moveControl)'
assert t.count(e1) == 1, 'E1'
t = t.replace(e1, e1 + '''
        .def("moveMany", &PyMfcWindow::moveMany, py::arg("items"),
             "items=[(id,x,y,w,h,visible)]，一次 DeferWindowPos 原子提交，滚动时不再一次一重绘")''')
e2 = '        .def("move", &PyMfcPanel::move)'
assert t.count(e2) == 1, 'E2'
t = t.replace(e2, e2 + '''
        .def("moveMany", &PyMfcPanel::moveMany, py::arg("items"))''')

io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('moveMany 已加')
