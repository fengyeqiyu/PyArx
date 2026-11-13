import io
P = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
t = io.open(P, encoding='utf-8-sig').read()

old = """        self._colw_now = None                 # 当前列宽（随侧板宽度变化重算）"""
new = """        self._colw_now = None                 # 当前列宽（随侧板宽度变化重算）
        self._sort_col, self._sort_asc = -1, True   # 点同一列二次反向"""
assert t.count(old) == 1
t = t.replace(old, new)

old2 = """        self.nodes = sorted(self.nodes, key=k)
        self.fill_table()
        self.rebuild(fit=False)
        self.win.setText(L_EDIT_COUNT, '已按「%s」排序' % COLS[col])"""
new2 = """        self._sort_asc = not self._sort_asc if col == self._sort_col else True
        self._sort_col = col
        self.nodes = sorted(self.nodes, key=k, reverse=not self._sort_asc)
        self.fill_table()
        self.rebuild(fit=False)
        self.win.setText(L_EDIT_COUNT, '按「%s」%s排序'
                         % (COLS[col], '升序' if self._sort_asc else '降序'))"""
assert t.count(old2) == 1
t = t.replace(old2, new2)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('排序方向切换')
