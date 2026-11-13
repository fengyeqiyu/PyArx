import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
old = """        int v = row < 0 ? -1 : (row >= total ? total - 1 : row);
        m_tblSel[id] = v;
        ensureVisible(id, v);"""
new = """        int v = row < 0 ? -1 : (row >= total ? total - 1 : row);
        const int prevSel = (m_tblSel.find(id) != m_tblSel.end()) ? m_tblSel[id] : -1;
        m_tblSel[id] = v;
        ensureVisible(id, v);"""
assert t.count(old) == 1
t = t.replace(old, new)

old2 = """                for (int i = 0; i < total; ++i)
                {
                    LVITEMW st = {};
                    st.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                    st.state = (i == v) ? (LVIS_SELECTED | LVIS_FOCUSED) : 0;
                    ::SendMessageW(h, LVM_SETITEMSTATE, static_cast<WPARAM>(i),
                                   reinterpret_cast<LPARAM>(&st));
                }
                if (v >= 0)
                    ::SendMessageW(h, LVM_ENSUREVISIBLE, static_cast<WPARAM>(v), FALSE);
            }
            ::InvalidateRect(h, nullptr, TRUE);"""
new2 = """                // 只动两行：清掉旧选中、置新选中。
                // 原先是 for 循环逐项 SetItemState，39 行就触发 39 次重画，
                // 屏幕上能看到蓝色一行行扫过去 —— 这就是"选中时拖影严重"。
                if (prevSel >= 0 && prevSel < total && prevSel != v)
                {
                    LVITEMW st = {};
                    st.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                    st.state = 0;
                    ::SendMessageW(h, LVM_SETITEMSTATE, static_cast<WPARAM>(prevSel),
                                   reinterpret_cast<LPARAM>(&st));
                }
                if (v >= 0 && v < total)
                {
                    LVITEMW st = {};
                    st.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                    st.state = LVIS_SELECTED | LVIS_FOCUSED;
                    ::SendMessageW(h, LVM_SETITEMSTATE, static_cast<WPARAM>(v),
                                   reinterpret_cast<LPARAM>(&st));
                    ::SendMessageW(h, LVM_ENSUREVISIBLE, static_cast<WPARAM>(v), FALSE);
                }
            }
            // erase 参数必须 FALSE：TRUE 会先把整块表格擦白再画，一帧一次闪
            ::InvalidateRect(h, nullptr, FALSE);"""
assert t.count(old2) == 1
t = t.replace(old2, new2)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('选行只重绘两行 + 不擦白')
