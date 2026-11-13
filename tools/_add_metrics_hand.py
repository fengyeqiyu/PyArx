import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

# 1. Host 层 tableMetrics（tableMetrics 未插入时补，已插入则跳过）
anchor = """    void setTableColumns(int id, const std::wstring& tabJoined) override
    {"""
if 'py::object tableMetrics(int id)' not in t:
    new = """    // 表格度量：返回 (行高, 表头高)。Python 用它在布局时把表格高度按整行对齐，
    // 否则最后一行会被裁掉一截（选中行露出一条蓝边，看起来像残留）。
    py::object tableMetrics(int id)
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

""" + anchor
    assert t.count(anchor) == 1
    t = t.replace(anchor, new)

# 2. 两个 wrapper
old_w1 = """    void setColumnWidth(int id, int col, int px) { m_host.setColumnWidth(id, col, px); }"""
new_w1 = """    void setColumnWidth(int id, int col, int px) { m_host.setColumnWidth(id, col, px); }
    py::object getTableMetrics(int id) { return m_host.tableMetrics(id); }"""
assert t.count(old_w1) == 1
t = t.replace(old_w1, new_w1)

old_w2 = """    void setColumnWidth(int id, int col, int px) { m_host->setColumnWidth(id, col, px); }"""
new_w2 = """    void setColumnWidth(int id, int col, int px) { m_host->setColumnWidth(id, col, px); }
    py::object getTableMetrics(int id) { return m_host->tableMetrics(id); }"""
assert t.count(old_w2) == 1
t = t.replace(old_w2, new_w2)

# 3. .def（两个类）
for cls in ('PyMfcWindow', 'PyMfcPanel'):
    key = '        .def("setColumnWidth", &%s::setColumnWidth, "设置某列宽度 px")\n' % cls
    assert t.count(key) == 1, cls
    if '&%s::getTableMetrics' % cls not in t:
        add = ('        .def("getTableMetrics", &%s::getTableMetrics, py::arg("id"),\n'
               '             "表格度量：(行高, 表头高)，用于把表格高度对齐整行")\n') % cls
        t = t.replace(key, key + add)

# 4. 光标：悬浮手形
old3 = """        ::SetCursor(::LoadCursorW(nullptr, st.drag ? IDC_SIZEALL : IDC_ARROW));"""
new3 = """        // 画布内悬浮显示手形（和网页 canvas 的 cursor:grab 一致），拖动中给四向箭头
        ::SetCursor(::LoadCursorW(nullptr, st.drag ? IDC_SIZEALL : IDC_HAND));"""
assert t.count(old3) == 1
t = t.replace(old3, new3)

old4 = """        ::SetCursor(::LoadCursorW(nullptr, IDC_SIZEALL));
        handled = true;
        return 0;
    }
    case WM_LBUTTONUP:"""
new4 = """        ::SetCursor(::LoadCursorW(nullptr, IDC_HAND));
        handled = true;
        return 0;
    }
    case WM_LBUTTONUP:"""
assert t.count(old4) == 1
t = t.replace(old4, new4)

io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('C++：tableMetrics + 手形光标 已改')
