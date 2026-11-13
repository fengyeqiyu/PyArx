import io
P = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
t = io.open(P, encoding='utf-8-sig').read()

old_c = "GRID_MAX_ROWS = 44               # 侧板高度能塞下的行数上限（行高最小 15）\n"
assert t.count(old_c) == 1, 'C1'
t = t.replace(old_c, old_c + "GRID_RH = 22                    # 侧板行高（固定，不再为塞下而压扁；超出靠滚轮）\n")

old_g = '''    def _grid_geom(self, n):
        """算行高：先把 39 行塞进侧板可用高度，行多时压到 15 再截断。"""
        avail = ED_H - ED_HEAD_H - 2 - ED_HEAD_H2 - 36
        rh = avail // max(1, n)
        rh = max(15, min(17, rh))
        rows = min(n, avail // rh)
        return rh, rows
'''
new_g = '''    def _grid_geom(self, n):
        """固定行高 22，返回 (行高, 可见行数)。超出可视区的行不再压扁/截断，改用滚轮。"""
        avail = ED_H - ED_HEAD_H - 2 - ED_HEAD_H2 - 36
        return GRID_RH, max(1, avail // GRID_RH)

    def _grid_body_rect(self):
        """侧板里“可滚动的行区”矩形（客户区坐标），滚轮只在这个矩形内生效。"""
        return (ED_X + ED_PAD, ED_Y + ED_HEAD_H + 2,
                ED_W - 2 * ED_PAD, ED_H - ED_HEAD_H - 2 - 34)
'''
assert t.count(old_g) == 1, 'C2'
t = t.replace(old_g, new_g)

i0 = t.index('    def fill_table(self):')
i1 = t.index('    def _grid_panel_ids(self):')
new_fill = '''    def fill_table(self):
        """HTML tbody：每行 7 个无边框输入 + 末列 ✕。
        行高固定 22，超出可视区的行照样建控件（collect_grid 要读到全部值），
        只是隐藏起来，靠滚轮改变 _scroll_row 再整体平移。"""
        win = self.win
        n = len(self.nodes)
        rh, vis = self._grid_geom(n)
        self._grid_rh = rh
        self._grid_vis = vis
        self._grid_rows = n
        lo, hi = 0, max(0, n - vis)
        self._scroll_row = min(hi, max(lo, getattr(self, '_scroll_row', 0)))
        off = self._scroll_row
        self._grid_ids = []
        tbl_x = ED_X + ED_PAD
        y0 = ED_Y + ED_HEAD_H + 2 + ED_HEAD_H2
        for i in range(n):
            node = self.nodes[i]
            x = tbl_x
            y = y0 + (i - off) * rh
            for j, key in enumerate(FIELDS):
                cid = GRID_BASE + i * 8 + j
                win.addEdit(cid, self._grid_text(node, key), x + 1, y + 1, COLW[j] - 2, rh - 2)
                win.setControlFont(cid, 'Microsoft YaHei', 11, False)
                self._grid_ids.append(cid)
                x += COLW[j]
            cid = GRID_BASE + i * 8 + 7
            win.addButton(cid, '✕', x, y + 1, COLW[7], rh - 2)
            win.setControlFont(cid, 'Microsoft YaHei', 11, False)
            win.onClick(cid, lambda code=node['code']: self.on_grid_delete(code))
            self._grid_ids.append(cid)
        win.raiseMany(self._grid_ids)
        self._grid_scroll_apply()
        win.setText(L_EDIT_COUNT, '共 %d 行' % n)

    def _grid_scroll_apply(self):
        """按 _scroll_row 把行控件挪到该在的 y，并只让可视区内的行可见。"""
        win = self.win
        n = getattr(self, '_grid_rows', 0)
        if n <= 0:
            return
        rh = getattr(self, '_grid_rh', GRID_RH)
        vis = getattr(self, '_grid_vis', 1)
        off = getattr(self, '_scroll_row', 0)
        tbl_x = ED_X + ED_PAD
        y0 = ED_Y + ED_HEAD_H + 2 + ED_HEAD_H2
        for i in range(n):
            show = off <= i < off + vis
            y = y0 + (i - off) * rh
            x = tbl_x
            for j in range(8):
                cid = GRID_BASE + i * 8 + j
                if show:
                    if j < 7:
                        win.move(cid, x + 1, y + 1, COLW[j] - 2, rh - 2)
                    else:
                        win.move(cid, x, y + 1, COLW[7], rh - 2)
                    win.setVisible(cid, True)
                else:
                    win.setVisible(cid, False)
                x += COLW[j]
        if not self.editing:
            for cid in getattr(self, '_grid_ids', []):
                win.setVisible(cid, False)

    def on_edit_wheel(self, delta):
        """宿主回调：侧板行区滚轮，delta 正数往下。"""
        if not self.editing:
            return
        n = getattr(self, '_grid_rows', 0)
        vis = getattr(self, '_grid_vis', 1)
        hi = max(0, n - vis)
        off = min(hi, max(0, getattr(self, '_scroll_row', 0) + int(delta)))
        if off != getattr(self, '_scroll_row', 0):
            self._scroll_row = off
            self._grid_scroll_apply()

'''
t = t[:i0] + new_fill + t[i1:]

old_e = "        w.onClick(B_CLOSE, self.on_close_editor)\n"
assert t.count(old_e) == 1, 'C4'
t = t.replace(old_e, old_e + "        w.onWheel(self.on_edit_wheel)\n")

old_s = '''        self.win.raiseMany(self._panel_ids())
        self.fill_table()
'''
assert t.count(old_s) == 1, 'C5'
t = t.replace(old_s, '''        self.win.raiseMany(self._panel_ids())
        self.fill_table()
        self.win.setWheelRegion(*self._grid_body_rect(), 3)   # 滚轮一档 = 3 行
''')

old_h = '''    def hide_editor(self):
        self.editing = False
'''
assert t.count(old_h) == 1, 'C6'
t = t.replace(old_h, '''    def hide_editor(self):
        self.editing = False
        self.win.setWheelRegion(0, 0, 0, 0, 0)      # 侧板关了就不吃滚轮
''')

io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('pp_app 侧板可滚动')
