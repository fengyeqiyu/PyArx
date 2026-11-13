import io
P = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
t = io.open(P, encoding='utf-8-sig').read()


def span(name):
    i = t.index('\n    def %s(' % name) + 1
    j = len(t)
    for pat in ('\n    def ', '\n\nclass ', '\n\n# ---', '\n\ndef '):
        k = t.find(pat, i + 1)
        if k != -1:
            j = min(j, k + 1)
    return i, j


def repl(name, code):
    global t
    i, j = span(name)
    t = t[:i] + code + t[j:]


FULL = '''    def _layout_editor(self, cw, ch):
        """侧板内部：头部（标题 / 3 按钮）→ 表格（原生 ListView 撑满）→ 行编辑条 → footer。"""
        ed_x = max(CV_X, cw - ED_W)
        ed_y = CV_Y
        ed_h = max(240, ch - CV_Y - 8)
        self._ed_rc = (ed_x, ed_y, ED_W, ed_h)
        px = ed_x + ED_PAD
        inner = ED_W - 2 * ED_PAD
        on = self._row_edit_open

        foot_y = ed_y + ed_h - 30
        strip_y = foot_y - 10 - 56                     # 行编辑条：两行（输入 / 按钮）
        items = [
            (ED_PANEL, ed_x, ed_y, ED_W, ed_h, True),
            (L_EDIT_TITLE, px, ed_y + 12, 120, 20, True),
            (L_EDIT_COUNT, px + 78, ed_y + 12, 100, 20, True),
            (B_CLOSE, ed_x + ED_W - ED_PAD - 72, ed_y + 8, 72, 26, True),
            (B_APPLY, ed_x + ED_W - ED_PAD - 172, ed_y + 8, 96, 26, True),
            (B_ADDROW, ed_x + ED_W - ED_PAD - 268, ed_y + 8, 92, 26, True),
            (L_EDIT_FOOT, px, foot_y, inner, 20, True),
        ]
        # 行编辑条：7 个输入按 HTML 的列宽排，下面一行 确定/删除/取消
        x = px
        for i in range(7):
            items.append((E_F0 + i, x, strip_y, COLW[i] - 2, 26, on))
            x += COLW[i]
        items += [(B_LOADROW, px, strip_y + 30, 72, 26, on),
                  (B_APPLYROW, px + 76, strip_y + 30, 72, 26, on),
                  (B_DELROW, px + 152, strip_y + 30, 72, 26, on)]
        # 表格：占满剩余高度，超出部分由 ListView 自己的滚动条处理
        tbl_y = ed_y + ED_HEAD_H + 2
        tbl_h = max(120, strip_y - 8 - tbl_y)
        items.append((TBL, px, tbl_y, inner, tbl_h, True))
        return items

    def on_resize(self, cw, ch):
        """宿主尺寸变化：按网格重算所有区块（窗口拉大后画布跟着变大）。"""
        self._layout()

    # ---------- 行编辑（双击表格某行） ----------
    def on_grid_dbl(self):
        r = self.win.getTableSelectedRow(TBL)
        if 0 <= r < len(self.nodes):
            self._open_row_edit(r)

    def _open_row_edit(self, r):
        w = self.win
        if not (0 <= r < len(self.nodes)):
            return
        self._row_edit_row = r
        self._row_edit_open = True
        for j in range(7):
            w.setText(E_F0 + j, w.getTableCell(TBL, r, j) or '')
        self._layout()
        ids = [E_F0 + i for i in range(7)] + [B_LOADROW, B_APPLYROW, B_DELROW]
        for i in ids:
            w.setVisible(i, True)
        w.raiseMany(ids)
        w.setText(L_EDIT_COUNT, '正在编辑第 %d 行' % (r + 1))

    def _row_edit_close(self):
        w = self.win
        if not getattr(self, '_row_edit_open', False):
            return
        self._row_edit_open = False
        for i in [E_F0 + i for i in range(7)] + [B_LOADROW, B_APPLYROW, B_DELROW]:
            w.setVisible(i, False)
        w.setText(L_EDIT_COUNT, '共 %d 行' % len(self.nodes))

    def _row_vals(self):
        return [(self.win.getText(E_F0 + i) or '').strip() for i in range(7)]

    def on_row_ok(self):
        r = getattr(self, '_row_edit_row', -1)
        if not (0 <= r < len(self.nodes)):
            return
        vals = self._row_vals()
        if not vals[0]:
            self.win.setText(L_EDIT_COUNT, '第 %d 行：编号不能为空' % (r + 1))
            return
        for k, lab in ((4, '负载'), (6, '长度')):
            try:
                f = float(vals[k] or 0)
            except ValueError:
                self.win.setText(L_EDIT_COUNT, '第 %d 行：%s必须是不小于 0 的数字' % (r + 1, lab))
                return
            if f < 0:
                self.win.setText(L_EDIT_COUNT, '第 %d 行：%s必须是不小于 0 的数字' % (r + 1, lab))
                return
        if vals[2] == vals[0]:
            self.win.setText(L_EDIT_COUNT, '第 %d 行：上游不能是节点自己' % (r + 1))
            return
        old = self.nodes[r]['code']
        nd = dict(code=vals[0], name=vals[1], upstream=vals[2], cable=vals[3],
                  loadKw=float(vals[4] or 0), voltage=vals[5] or '380',
                  length=float(vals[6] or 0))
        self.nodes[r] = nd
        if old != nd['code']:
            for n in self.nodes:
                if (n.get('upstream') or '') == old:
                    n['upstream'] = nd['code']
            self.collapsed.discard(old)
            if self.cur_sel == old:
                self.cur_sel = nd['code']
        # 单行写回 ListView（不重排、不丢滚动位置）
        for j in range(7):
            self.win.setTableCell(TBL, r, j, self._grid_text(nd, FIELDS[j]))
        self._row_edit_close()
        self.rebuild(fit=False)
        if self.cur_sel == nd['code']:
            self.fill_detail(nd['code'])
        self.win.setText(L_EDIT_COUNT, '第 %d 行已更新' % (r + 1))

    def on_row_del(self):
        r = getattr(self, '_row_edit_row', -1)
        if not (0 <= r < len(self.nodes)):
            return
        code = self.nodes[r]['code']
        self._row_edit_close()
        self.on_grid_delete(code)

    def on_row_cancel(self):
        self._row_edit_close()

'''
repl('_layout_editor', FULL)
repl('_layout_editor_tail', '')

io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('layout + 行编辑完成')
