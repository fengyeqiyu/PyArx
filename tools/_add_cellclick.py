import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
old = """            // 双击单元格：把 (行, 列) 回给 Python，由它把 EDIT 盖到该格上原地编辑
            if (msg == WM_LBUTTONDBLCLK && hi.iItem >= 0)
                host->fireGridAction("dbl", hi.iItem, hi.iSubItem, L"");"""
new = """            if (hi.iItem >= 0)
            {
                // 单击别的格：先让 Python 把上一格的原地编辑收尾（提交）。
                // 不能只靠 EN_KILLFOCUS —— 焦点若本来就不在编辑框上，失焦通知不会来。
                if (msg == WM_LBUTTONDOWN)
                    host->fireGridAction("cellclick", hi.iItem, hi.iSubItem, L"");
                // 双击单元格：把 (行, 列) 回给 Python，由它把 EDIT 盖到该格上原地编辑
                else
                    host->fireGridAction("dbl", hi.iItem, hi.iSubItem, L"");
            }"""
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)

P2 = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
s = io.open(P2, encoding='utf-8').read()
old2 = """        if action == 'dbl':
            if 0 <= row < n:
                self._cell_begin(row, int(col))"""
new2 = """        if action == 'cellclick':
            # 单击表格任何格：先把上一格的原地编辑提交掉（点别处=确认）
            if getattr(self, '_ce', None):
                self._cell_end(commit=True)
        elif action == 'dbl':
            if 0 <= row < n:
                self._cell_begin(row, int(col))"""
assert s.count(old2) == 1
s = s.replace(old2, new2)
io.open(P2, 'w', encoding='utf-8').write(s)
print('单击表格回抛 cellclick + Python 收尾')
