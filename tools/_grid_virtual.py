import io
P = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
t = io.open(P, encoding='utf-8-sig').read()

i0 = t.index('    def fill_table(self):')
i1 = t.index('    def _grid_panel_ids(self):')
new = '''    def fill_table(self):
        """HTML tbody：每行 7 个无边框输入 + 末列 ✕。
        只建"一屏放得下"的槽位（slot），滚轮时把下一屏的数据重新绑到同样的控件上，
        不移动任何窗口 —— 逐条 move 会反复触发父窗口补画空白，那就是滚动时闪的原因。"""
        win = self.win
        n = len(self.nodes)
        rh, vis = self._grid_geom(n)
        self._grid_rh = rh
        self._grid_rows = n
        self._grid_draft = [[self._grid_text(node, key) for key in FIELDS] for node in self.nodes]
        slots = min(n, vis) if n > 0 else 0
        self._grid_vis = slots
        self._grid_slots = slots
        lo, hi = 0, max(0, n - slots)
        self._scroll_row = min(hi, max(lo, getattr(self, '_scroll_row', 0)))
        self._slot_shown = [False] * slots
        self._grid_ids = []
        tbl_x = ED_X + ED_PAD
        y0 = ED_Y + ED_HEAD_H + 2 + ED_HEAD_H2
        for s in range(slots):
            x = tbl_x
            y = y0 + s * rh
            for j in range(7):
                cid = GRID_BASE + s * 8 + j
                win.addEdit(cid, '', x + 1, y + 1, COLW[j] - 2, rh - 2)
                win.setControlFont(cid, 'Microsoft YaHei', 11, False)
                self._grid_ids.append(cid)
                x += COLW[j]
            cid = GRID_BASE + s * 8 + 7
            win.addButton(cid, '✕', x, y + 1, COLW[7], rh - 2)
            win.setControlFont(cid, 'Microsoft YaHei', 11, False)
            win.onClick(cid, lambda slot=s: self.on_grid_delete_slot(slot))
            self._grid_ids.append(cid)
        win.raiseMany(self._grid_ids)
        self._grid_bind_slots(force=True)
        win.setText(L_EDIT_COUNT, '共 %d 行' % n)

    def _grid_bind_slots(self, force=False):
        """把 _grid_draft 里 _scroll_row 开始的这一段数据绑到槽位控件上。"""
        win = self.win
        off = getattr(self, '_scroll_row', 0)
        n = len(getattr(self, '_grid_draft', []))
        for s in range(getattr(self, '_grid_slots', 0)):
            i = off + s
            has = i < n
            if has:
                row = self._grid_draft[i]
                for j in range(7):
                    win.setText(GRID_BASE + s * 8 + j, row[j])
            if force or has != self._slot_shown[s]:
                self._slot_shown[s] = has
                for j in range(8):
                    win.setVisible(GRID_BASE + s * 8 + j, has and self.editing)

    def _grid_flush_slots(self):
        """把当前槽位里改过的文字写回 draft（未显示的行保持 draft 原值）。"""
        win = self.win
        off = getattr(self, '_scroll_row', 0)
        n = len(getattr(self, '_grid_draft', []))
        for s in range(getattr(self, '_grid_slots', 0)):
            i = off + s
            if i >= n:
                continue
            row = self._grid_draft[i]
            for j in range(7):
                row[j] = win.getText(GRID_BASE + s * 8 + j) or ''

    def on_edit_wheel(self, delta):
        """宿主回调：侧板行区滚轮，delta 正数往下。只换数据，不搬窗口。"""
        if not self.editing:
            return
        n = len(getattr(self, '_grid_draft', []))
        slots = getattr(self, '_grid_slots', 0)
        if slots <= 0 or n <= slots:
            return
        hi = n - slots
        off = min(hi, max(0, getattr(self, '_scroll_row', 0) + int(delta)))
        if off == getattr(self, '_scroll_row', 0):
            return
        self._grid_flush_slots()
        self._scroll_row = off
        self._grid_bind_slots()

    def on_grid_delete_slot(self, slot):
        """✕ 是按槽位注册的，真正要删的是这个槽位当前显示的那一行。"""
        self._grid_flush_slots()
        i = getattr(self, '_scroll_row', 0) + slot
        draft = getattr(self, '_grid_draft', [])
        if 0 <= i < len(draft):
            self.on_grid_delete((draft[i][0] or '').strip())

'''
t = t[:i0] + new + t[i1:]

old_c = '''    def collect_grid(self):
        """把行内控件里的值读回来；返回 (nodes, 错误文案)。"""
        win = self.win
        out = []
        for i in range(getattr(self, '_grid_rows', 0)):
            vals = [(win.getText(GRID_BASE + i * 8 + j) or '').strip() for j in range(7)]
'''
new_c = '''    def collect_grid(self):
        """把全部行的值读回来；返回 (nodes, 错误文案)。
        槽位只覆盖一屏，所以先从槽位 flush 回 draft，再按 draft 校验（含未显示的行）。"""
        self._grid_flush_slots()
        out = []
        for i, vals in enumerate(getattr(self, '_grid_draft', [])):
            vals = [(v or '').strip() for v in vals]
'''
assert t.count(old_c) == 1, 'collect'
t = t.replace(old_c, new_c)

io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('侧板改为槽位复用')
