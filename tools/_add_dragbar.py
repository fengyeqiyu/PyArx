import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

# 1) Host：拖动条回调
old = """    // 原地编辑：EDIT 失焦 → 提交（点别处就等于确认，和 Excel 一致）"""
new = """    // 可拖动分割条（表格侧板与画布之间）：鼠标按下后按屏幕位移回调 Python 改布局
    std::map<int, py::object> m_drag;
    void onDrag(int id, py::object cb) { m_drag[id] = std::move(cb); }
    void fireDrag(int id, const char* phase, int x)
    {
        auto it = m_drag.find(id);
        if (it == m_drag.end())
            return;
        py::gil_scoped_acquire gil;
        try { it->second(std::string(phase), x); }
        catch (const py::error_already_set& e) { debugLogf(L"[drag] %hs", e.what()); }
    }

    // 原地编辑：EDIT 失焦 → 提交（点别处就等于确认，和 Excel 一致）"""
assert t.count(old) == 1
t = t.replace(old, new)

# 2) 拖动条窗口过程（放在 PyMfcOwnerDrawProc 之前）
old = """LRESULT CALLBACK PyMfcOwnerDrawProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)"""
new = """// 可拖动分割条：一个 6px 的 owner-draw STATIC，鼠标按下后捕获鼠标，
// 位移回调给 Python（它按位移改侧板宽度并重排），其余消息照旧转给原过程保证还会画。
static LRESULT CALLBACK PyMfcDragBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
        ::SetCapture(hwnd);
        if (host != nullptr)
            host->fireDrag(id, "down", GET_X_LPARAM(lParam));
        return 0;
    case WM_MOUSEMOVE:
        if (::GetCapture() == hwnd && host != nullptr)
            host->fireDrag(id, "move", GET_X_LPARAM(lParam));
        return 0;
    case WM_LBUTTONUP:
        if (host != nullptr)
            host->fireDrag(id, "up", GET_X_LPARAM(lParam));
        if (::GetCapture() == hwnd)
            ::ReleaseCapture();
        return 0;
    default:
        break;
    }
    auto it = pyMfcSkinOldProcs().find(hwnd);
    WNDPROC old = (it != pyMfcSkinOldProcs().end()) ? it->second
                                                    : ::DefWindowProcW;
    return ::CallWindowProcW(old, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK PyMfcOwnerDrawProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)"""
assert t.count(old) == 1
t = t.replace(old, new)

# 3) onDrag 安装过程：给控件换 proc 并记下原过程
old = """    void setWheelRegion(int x, int y, int w, int h, int step)
    {"""
new = """    // 把 id 对应的控件变成可拖分割条（记住原过程，转发非鼠标消息）
    bool installDragBar(int id)
    {
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return false;
        WNDPROC old = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(
            h, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&PyMfcDragBarProc)));
        if (old == nullptr)
            return false;
        pyMfcSkinOldProcs()[h] = old;
        ::SetPropW(h, kSkinHostProp, reinterpret_cast<HANDLE>(this));
        return true;
    }

    void setWheelRegion(int x, int y, int w, int h, int step)
    {"""
assert t.count(old) == 1
t = t.replace(old, new)

# 4) wrapper
old = """    void onBlur(int id, py::object cb) { m_host.onBlur(id, cb); }"""
new = """    void onBlur(int id, py::object cb) { m_host.onBlur(id, cb); }
    void onDrag(int id, py::object cb) { m_host.onDrag(id, cb); m_host.installDragBar(id); }"""
assert t.count(old) == 1
t = t.replace(old, new)
old = """    void onBlur(int id, py::object cb) { m_host->onBlur(id, cb); }"""
new = """    void onBlur(int id, py::object cb) { m_host->onBlur(id, cb); }
    void onDrag(int id, py::object cb) { m_host->onDrag(id, cb); m_host->installDragBar(id); }"""
assert t.count(old) == 1
t = t.replace(old, new)

# 5) .def
for cls in ('PyMfcWindow', 'PyMfcPanel'):
    key = ('        .def("onBlur", &%s::onBlur, py::arg("id"), py::arg("callback"),\n'
           '             "EDIT 失焦回调（原地单元格编辑提交用）")\n') % cls
    assert t.count(key) == 1, cls
    add = ('        .def("onDrag", &%s::onDrag, py::arg("id"), py::arg("callback"),\n'
           '             "可拖分割条：回调 (phase, x)，phase=down/move/up")\n') % cls
    t = t.replace(key, key + add)

io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('C++：拖动条过程 + onDrag 已加')
