import io
p = 'G:/PyArx/PyMfcXaml/PyMfcHost.h'
s = io.open(p, encoding='utf-8').read()
old = "void addTableRow(int id, const std::wstring& tabJoined) override;"
new = """void addTableRow(int id, const std::wstring& tabJoined) override;
// 一次灌满整表（行间 \\n、格间 \\t）：39 次逐行插入 = 39 次重绘 ≈ 22 ms，
// 合成一次插入 + 一次失效后只重画一遍。
void setTableRows(int id, const std::wstring& blob);"""
assert s.count(old) == 1, s.count(old)
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8', newline='').write(s)

p = 'G:/PyArx/PyMfcXaml/PyMfcHost.cpp'
s = io.open(p, encoding='utf-8-sig').read()
anchor = """void PyMfcHost::clearTableRows(int id)"""
impl = """void PyMfcHost::setTableRows(int id, const std::wstring& blob)
{
        // 行间 '\\n'、格间 '\\t'。整表一次灌进去：逐行 addTableRow 时每条都在控件
        // 重绘开启状态下插行（39 行实测 22.5 ms），这里 WM_SETREDRAW(FALSE) 包住
        // 全部插入，只在最后失效一次。
        std::vector<std::vector<std::wstring>> rows;
        std::wstring cur;
        auto flushRow = [&]() {
            rows.push_back(splitTabs(cur));
            cur.clear();
        };
        for (wchar_t ch : blob)
        {
            if (ch == L'\\n')
                flushRow();
            else if (ch != L'\\r')
                cur.push_back(ch);
        }
        if (!cur.empty())
            flushRow();

        m_tblRows[id] = rows;
        m_tblSel[id] = -1;
        m_tblScroll[id] = 0;

        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr)
            return;
        wchar_t cls[64] = {};
        ::GetClassNameW(h, cls, 64);
        if (_wcsicmp(cls, L"SysListView32") == 0)
        {
            ::SendMessageW(h, WM_SETREDRAW, FALSE, 0);
            ::SendMessageW(h, LVM_DELETEALLITEMS, 0, 0);
            for (size_t r = 0; r < rows.size(); ++r)
            {
                const std::vector<std::wstring>& cells = rows[r];
                LVITEMW it = {};
                it.mask = LVIF_TEXT;
                it.iItem = static_cast<int>(r);
                it.pszText = const_cast<LPWSTR>(cells.empty() ? L"" : cells[0].c_str());
                ::SendMessageW(h, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&it));
                for (size_t c = 1; c < cells.size(); ++c)
                {
                    LVITEMW sub = {};
                    sub.iSubItem = static_cast<int>(c);
                    sub.pszText = const_cast<LPWSTR>(cells[c].c_str());
                    ::SendMessageW(h, LVM_SETITEMTEXTW, static_cast<WPARAM>(r),
                                   reinterpret_cast<LPARAM>(&sub));
                }
            }
            ::SendMessageW(h, WM_SETREDRAW, TRUE, 0);
        }
        ::InvalidateRect(h, nullptr, TRUE);
}

void PyMfcHost::clearTableRows(int id)"""
assert s.count(anchor) == 1
s = s.replace(anchor, impl)
io.open(p, 'w', encoding='utf-8-sig', newline='').write(s)
print('host: setTableRows 已加')
