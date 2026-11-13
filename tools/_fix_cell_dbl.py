import io
P = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
s = io.open(P, encoding='utf-8').read()

# 1) 双击不再打开底部编辑条，走原地编辑
old = """    def on_grid_dbl(self):
        r = self.win.getTableSelectedRow(TBL)
        if 0 <= r < len(self.nodes):
            self._open_row_edit(r)"""
new = """    def on_grid_dbl(self):
        \"\"\"双击表格：直接在那一格里原地编辑（底部编辑条不再弹出来抢视线）。\"\"\"
        if getattr(self, '_ce', None):
            return                       # 宿主的 'dbl' 回调已经进入了原地编辑
        r = self.win.getTableSelectedRow(TBL)
        if 0 <= r < len(self.nodes):
            self._cell_begin(r, 0)"""
assert s.count(old) == 1
s = s.replace(old, new)

# 2) 布局不要动正在原地编辑的那个框
old = """        x = px
        for i in range(7):
            items.append((E_F0 + i, x, strip_y, widths[i] - 2, 26, on))
            x += widths[i]"""
new = """        x = px
        cel = getattr(self, '_ce', None)
        for i in range(7):
            if cel and cel[2] == E_F0 + i:
                x += widths[i]
                continue                 # 正在原地编辑：位置由 _cell_begin 自己管，别被布局挪走
            items.append((E_F0 + i, x, strip_y, widths[i] - 2, 26, on))
            x += widths[i]"""
assert s.count(old) == 1
s = s.replace(old, new)

io.open(P, 'w', encoding='utf-8').write(s)
print('双击改走原地编辑 + 布局不吃编辑框')
