import io
P = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
t = io.open(P, encoding='utf-8-sig').read()

i0 = t.index('    def _grid_scroll_apply(self):')
i1 = t.index('    def on_edit_wheel(self, delta):')
new = '''    def _grid_scroll_apply(self):
        """按 _scroll_row 一次性把行控件摆好。
        逐条 move/setVisible 会让每次调用都补画一次父窗口空白，312 个控件滚一下就是一片闪烁，
        所以统一走 moveMany（DeferWindowPos 原子提交，只重绘一次）。"""
        win = self.win
        n = getattr(self, '_grid_rows', 0)
        if n <= 0:
            return
        rh = getattr(self, '_grid_rh', GRID_RH)
        vis = getattr(self, '_grid_vis', 1)
        off = getattr(self, '_scroll_row', 0)
        editing = self.editing
        tbl_x = ED_X + ED_PAD
        y0 = ED_Y + ED_HEAD_H + 2 + ED_HEAD_H2
        items = []
        for i in range(n):
            show = editing and off <= i < off + vis
            y = y0 + (i - off) * rh
            x = tbl_x
            for j in range(8):
                cid = GRID_BASE + i * 8 + j
                if j < 7:
                    items.append((cid, x + 1, y + 1, COLW[j] - 2, rh - 2, show))
                else:
                    items.append((cid, x, y + 1, COLW[7], rh - 2, show))
                x += COLW[j]
        if items:
            win.moveMany(items)

'''
t = t[:i0] + new + t[i1:]
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('滚动改为原子提交')
