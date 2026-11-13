import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

a = "    void onClick(int id, py::object callback) { m_click[id] = std::move(callback); }\n"
assert t.count(a) == 1, 'A'
A = a + '''
    // ---- 侧板滚轮 ----------------------------------------------------------
    // 侧板里全是 EDIT 子控件，滚轮只会发给“焦点窗口”（某个 EDIT），EDIT 不处理时
    // 由 DefWindowProc 冒泡到宿主窗口。所以在这一层拦最可靠：命中注册的矩形就折算成
    // 一个“滚动档”回调给 Python，由 Python 重排行的位置（MFCXaml 不会替子控件滚动）。
    void setWheelRegion(int x, int y, int w, int h, int step)
    {
        m_wheelOn = (w > 0 && h > 0);
        m_wheelRect.left = x; m_wheelRect.top = y;
        m_wheelRect.right = x + w; m_wheelRect.bottom = y + h;
        m_wheelStep = step > 0 ? step : 1;
    }
    void onWheel(py::object cb) { m_wheelCb = std::move(cb); }
    bool fireWheelMessage(WPARAM wp, LPARAM lp)
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
    RECT m_wheelRect = {0, 0, 0, 0};
    bool m_wheelOn = false;
    int m_wheelStep = 3;
    py::object m_wheelCb;
'''
t = t.replace(a, A)

b = "LRESULT CALLBACK PyMfcHost::animHostProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)\n{"
assert t.count(b) == 1, 'B'
B = '''// 宿主主窗口的滚轮拦截：没命中注册区域时原样交回 MFC 的 AfxWndProc。
static std::map<HWND, WNDPROC>& wheelOldProcs()
{
    static std::map<HWND, WNDPROC> table;
    return table;
}
static const wchar_t* kWheelHostProp = L"PyMfcWheelHost";
static LRESULT CALLBACK PyMfcWheelProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto it = wheelOldProcs().find(hwnd);
    WNDPROC old = it != wheelOldProcs().end() ? it->second : nullptr;
    if (msg == WM_MOUSEWHEEL)
    {
        PyMfcHost* host = reinterpret_cast<PyMfcHost*>(::GetPropW(hwnd, kWheelHostProp));
        if (host != nullptr && host->fireWheelMessage(wp, lp))
            return 0;
    }
    return old != nullptr ? ::CallWindowProcW(old, hwnd, msg, wp, lp)
                          : ::DefWindowProcW(hwnd, msg, wp, lp);
}

''' + b
t = t.replace(b, B)

c = '''            nullptr, nullptr, AfxGetInstanceHandle(), nullptr);
        if (hwnd == nullptr)
            return false;
        m_wnd.SubclassWindow(hwnd);
'''
assert t.count(c) == 1, 'C'
t = t.replace(c, c + '''        // 滚轮拦截挂在 MFC 子类化之后：old proc 就是 AfxWndProc，没命中会原样转回
        ::SetPropW(hwnd, kWheelHostProp, reinterpret_cast<HANDLE>(&m_host));
        wheelOldProcs()[hwnd] = reinterpret_cast<WNDPROC>(
            ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&PyMfcWheelProc)));
''' )

d1 = "    void onClick(int id, py::object cb) { m_host.onClick(id, std::move(cb)); }\n"
assert t.count(d1) == 1, 'D1'
t = t.replace(d1, d1 + '''    void setWheelRegion(int x, int y, int w, int h, int step) { m_host.setWheelRegion(x, y, w, h, step); }
    void onWheel(py::object cb) { m_host.onWheel(std::move(cb)); }
''')
d2 = "    void onClick(int id, py::object cb) { m_host->onClick(id, std::move(cb)); }\n"
assert t.count(d2) == 1, 'D2'
t = t.replace(d2, d2 + '''    void setWheelRegion(int x, int y, int w, int h, int step) { m_host->setWheelRegion(x, y, w, h, step); }
    void onWheel(py::object cb) { m_host->onWheel(std::move(cb)); }
''')

e1 = '        .def("onClick", &PyMfcWindow::onClick)'
assert t.count(e1) == 1, 'E1'
t = t.replace(e1, e1 + '''
        .def("setWheelRegion", &PyMfcWindow::setWheelRegion, py::arg("x"), py::arg("y"),
             py::arg("w"), py::arg("h"), py::arg("step") = 3,
             "注册收滚轮的区域（客户区坐标）：鼠标在里面滚动就回调 onWheel(±step)")
        .def("onWheel", &PyMfcWindow::onWheel, py::arg("cb"),
             "cb(delta)，delta 为“往下为正”的行增量；MFCXaml 不替子控件滚动，行位置自己算")''')
e2 = '        .def("onClick", &PyMfcPanel::onClick)'
assert t.count(e2) == 1, 'E2'
t = t.replace(e2, e2 + '''
        .def("setWheelRegion", &PyMfcPanel::setWheelRegion, py::arg("x"), py::arg("y"),
             py::arg("w"), py::arg("h"), py::arg("step") = 3)
        .def("onWheel", &PyMfcPanel::onWheel, py::arg("cb"))''')

io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('C++ 补丁完成')
