import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

# 1) Host 层：失焦回调 + 单元格矩形
old = """    void setWheelRegion(int x, int y, int w, int h, int step)
    {"""
new = """    // 原地编辑：EDIT 失焦 → 提交（点别处就等于确认，和 Excel 一致）
    std::map<int, py::object> m_blur;
    void onBlur(int id, py::object cb) { m_blur[id] = std::move(cb); }
    void fireBlur(int id)
    {
        auto it = m_blur.find(id);
        if (it == m_blur.end())
            return;
        py::gil_scoped_acquire gil;
        try { it->second(); }
        catch (const py::error_already_set& e) { debugLogf(L"[blur] %hs", e.what()); }
    }

    // 单元格矩形（ListView 客户区坐标）：原地编辑要把 EDIT 精确盖到那一格上
    py::object cellRect(int id, int row, int col)
    {
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr || !::IsWindow(h))
            return py::none();
        RECT r = { 0, 0, 0, 0 };
        r.left = col;                       // LVM_GETSUBITEMRECT 用 left 成员传子项号
        ::SendMessageW(h, LVM_GETSUBITEMRECT, static_cast<WPARAM>(row),
                       reinterpret_cast<LPARAM>(&r));
        return py::make_tuple(r.left, r.top, r.right - r.left, r.bottom - r.top);
    }

    void setWheelRegion(int x, int y, int w, int h, int step)
    {"""
assert t.count(old) == 1
t = t.replace(old, new)

# 2) 消息映射 + 声明
old2 = """    ON_CONTROL_RANGE(EN_CHANGE, kFirstId, kLastId, &PyMfcHost::OnEditChange)"""
new2 = """    ON_CONTROL_RANGE(EN_CHANGE, kFirstId, kLastId, &PyMfcHost::OnEditChange)
    ON_CONTROL_RANGE(EN_KILLFOCUS, kFirstId, kLastId, &PyMfcHost::OnEditBlur)"""
assert t.count(old2) == 1
t = t.replace(old2, new2)

old3 = """    afx_msg void OnEditChange(UINT id);"""
new3 = """    afx_msg void OnEditChange(UINT id);
    afx_msg void OnEditBlur(UINT id);"""
assert t.count(old3) == 1
t = t.replace(old3, new3)

old4 = """void PyMfcHost::OnEditChange(UINT id)
{
    auto it = m_change.find(id);
    if (it != m_change.end())
        invokePython(it->second);
}"""
new4 = """void PyMfcHost::OnEditChange(UINT id)
{
    auto it = m_change.find(id);
    if (it != m_change.end())
        invokePython(it->second);
}

void PyMfcHost::OnEditBlur(UINT id)
{
    fireBlur(static_cast<int>(id));
}"""
assert t.count(old4) == 1
t = t.replace(old4, new4)

# 3) 表格双击 → 回抛行列
old5 = """        if (msg == WM_LBUTTONDOWN)
        {
            LVHITTESTINFO hi = {};
            hi.pt.x = GET_X_LPARAM(lParam);
            hi.pt.y = GET_Y_LPARAM(lParam);
            ::SendMessageW(hwnd, LVM_SUBITEMHITTEST, 0, reinterpret_cast<LPARAM>(&hi));
            host->m_gridCell[id] = std::make_pair(hi.iItem, hi.iSubItem);
        }"""
new5 = """        if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK)
        {
            LVHITTESTINFO hi = {};
            hi.pt.x = GET_X_LPARAM(lParam);
            hi.pt.y = GET_Y_LPARAM(lParam);
            ::SendMessageW(hwnd, LVM_SUBITEMHITTEST, 0, reinterpret_cast<LPARAM>(&hi));
            host->m_gridCell[id] = std::make_pair(hi.iItem, hi.iSubItem);
            // 双击单元格：把 (行, 列) 回给 Python，由它把 EDIT 盖到该格上原地编辑
            if (msg == WM_LBUTTONDBLCLK && hi.iItem >= 0)
                host->fireGridAction("dbl", hi.iItem, hi.iSubItem, L"");
        }"""
assert t.count(old5) == 1
t = t.replace(old5, new5)

# 4) EDIT 里回车/ESC → Python 提交/取消
old6 = """    // 状态类消息之后重画自己：内容/选中/滚动/焦点都会改变外观。"""
new6 = """    // 原地单元格编辑：EDIT 上按回车（确认）/ Esc（取消）直接回抛给 Python。
    // 不回抛就只能靠失焦提交，用户按回车会"响一声但没反应"。
    if (host != nullptr && msg == WM_KEYDOWN &&
        (wParam == VK_RETURN || wParam == VK_ESCAPE))
    {
        wchar_t clsName2[64] = {};
        ::GetClassNameW(hwnd, clsName2, 64);
        if (_wcsicmp(clsName2, L"Edit") == 0)
        {
            wchar_t buf[1024] = {};
            ::GetWindowTextW(hwnd, buf, 1024);
            host->fireGridAction("cellend", ::GetDlgCtrlID(hwnd),
                                 static_cast<int>(wParam), std::wstring(buf));
            return 0;
        }
    }

    // 状态类消息之后重画自己：内容/选中/滚动/焦点都会改变外观。"""
assert t.count(old6) == 1
t = t.replace(old6, new6)

# 5) wrappers
old7 = """    void setColumnWidth(int id, int col, int px) { m_host.setColumnWidth(id, col, px); }
    py::object getTableMetrics(int id) { return m_host.tableMetrics(id); }"""
new7 = """    void setColumnWidth(int id, int col, int px) { m_host.setColumnWidth(id, col, px); }
    py::object getTableMetrics(int id) { return m_host.tableMetrics(id); }
    void onBlur(int id, py::object cb) { m_host.onBlur(id, cb); }
    py::object cellRect(int id, int row, int col) { return m_host.cellRect(id, row, col); }"""
assert t.count(old7) == 1
t = t.replace(old7, new7)

old8 = """    void setColumnWidth(int id, int col, int px) { m_host->setColumnWidth(id, col, px); }
    py::object getTableMetrics(int id) { return m_host->tableMetrics(id); }"""
new8 = """    void setColumnWidth(int id, int col, int px) { m_host->setColumnWidth(id, col, px); }
    py::object getTableMetrics(int id) { return m_host->tableMetrics(id); }
    void onBlur(int id, py::object cb) { m_host->onBlur(id, cb); }
    py::object cellRect(int id, int row, int col) { return m_host->cellRect(id, row, col); }"""
assert t.count(old8) == 1
t = t.replace(old8, new8)

# 6) .def
for cls in ('PyMfcWindow', 'PyMfcPanel'):
    key = ('        .def("getTableMetrics", &%s::getTableMetrics, py::arg("id"),\n'
           '             "表格度量：(行高, 表头高)，用于把表格高度对齐整行")\n') % cls
    assert t.count(key) == 1, cls
    add = ('        .def("onBlur", &%s::onBlur, py::arg("id"), py::arg("callback"),\n'
           '             "EDIT 失焦回调（原地单元格编辑提交用）")\n'
           '        .def("cellRect", &%s::cellRect, py::arg("id"), py::arg("row"), py::arg("col"),\n'
           '             "单元格矩形 (x, y, w, h)，ListView 客户区坐标")\n') % (cls, cls)
    t = t.replace(key, key + add)

io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('C++：双击回抛列 / cellRect / onBlur / 回车提交 已加')
