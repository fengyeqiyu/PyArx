import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

anchor = """    void setTableColumns(int id, const std::wstring& tabJoined) override"""
COLW = """    // 列宽自适应：侧板宽度变化时按比例重设列宽（不清数据、不重建控件）
    void setColumnWidth(int id, int col, int px)
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

"""
assert t.count(anchor) == 1
t = t.replace(anchor, COLW + anchor, 1)

old5 = "    void addTableRow(int id, const std::wstring& s) { m_host.addTableRow(id, s); }"
new5 = ("    void addTableRow(int id, const std::wstring& s) { m_host.addTableRow(id, s); }\n"
        "    void setColumnWidth(int id, int col, int px) { m_host.setColumnWidth(id, col, px); }")
assert t.count(old5) == 1
t = t.replace(old5, new5)

old6 = "    void addTableRow(int id, const std::wstring& s) { m_host->addTableRow(id, s); }"
new6 = ("    void addTableRow(int id, const std::wstring& s) { m_host->addTableRow(id, s); }\n"
        "    void setColumnWidth(int id, int col, int px) { m_host->setColumnWidth(id, col, px); }")
assert t.count(old6) == 1
t = t.replace(old6, new6)

old7 = '        .def("onGridAction", &PyMfcWindow::onGridAction,\n             "表格按键/排序回调 cb(action, row, col, text)")'
new7 = old7 + '\n        .def("setColumnWidth", &PyMfcWindow::setColumnWidth, "设置某列宽度 px")'
assert t.count(old7) == 1
t = t.replace(old7, new7)

old8 = '        .def("onGridAction", &PyMfcPanel::onGridAction,\n             "表格按键/排序回调 cb(action, row, col, text)")'
new8 = old8 + '\n        .def("setColumnWidth", &PyMfcPanel::setColumnWidth, "设置某列宽度 px")'
assert t.count(old8) == 1
t = t.replace(old8, new8)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('setColumnWidth 已加')
