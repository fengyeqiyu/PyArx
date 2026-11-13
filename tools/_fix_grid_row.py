import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
old = """            auto it = host->m_gridCell.find(id);
            const int row = it != host->m_gridCell.end() ? it->second.first : -1;
            const int col = it != host->m_gridCell.end() ? it->second.second : 0;"""
new = """            // 行优先取"当前选中项"（用户点过行就一定有选中），点过单元格时用它补齐列
            int row = static_cast<int>(::SendMessageW(hwnd, LVM_GETNEXTITEM,
                          static_cast<WPARAM>(-1), LVNI_SELECTED));
            auto it = host->m_gridCell.find(id);
            if (row < 0 && it != host->m_gridCell.end())
                row = it->second.first;
            const int col = (it != host->m_gridCell.end() && it->second.first >= 0)
                            ? it->second.second : 0;"""
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P,'w',encoding='utf-8-sig',newline='').write(t)
print('行取自选中项')
