import io
P = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
t = io.open(P, encoding='utf-8-sig').read()

old = """        w.onClick(B_LOADROW, self.on_row_ok)
        w.onClick(B_APPLYROW, self.on_row_del)
        w.onClick(B_DELROW, self.on_row_cancel)"""
new = """        w.onClick(B_LOADROW, self.on_row_ok)
        w.onClick(B_APPLYROW, self.on_row_del)
        w.onClick(B_DELROW, self.on_row_cancel)
        # 表格的键盘与列头动作（F2/Delete/Insert/Ctrl+C/V/点列头排序）在宿主里拦，
        # 动作回到这里处理 —— 数据源始终在 Python 这一侧。
        if hasattr(w, 'onGridAction'):
            w.onGridAction(self.on_grid_action)"""
assert t.count(old) == 1, 'bind'
t = t.replace(old, new)

anchor = "    def on_row_cancel(self):"
NEW = '''    def on_row_cancel(self):
        self._row_edit_close()

    # ---------- 表格键盘 / 排序 / 粘贴 ----------
    def on_grid_action(self, action, row, col, text):
        """宿主回调：F2=edit，Delete=delete，Insert=insert，Ctrl+V=paste，点列头=sort。"""
        n = len(self.nodes)
        r = row if 0 <= row < n else self.win.getTableSelectedRow(TBL)
        if action == 'edit':
            if 0 <= r < n:
                self._open_row_edit(r)
        elif action == 'delete':
            if 0 <= r < n:
                self.on_grid_delete(self.nodes[r]['code'])
        elif action == 'insert':
            self.on_add_row()
        elif action == 'paste':
            self._grid_paste(text or '')
        elif action == 'sort':
            self._grid_sort(int(col or 0))

    @staticmethod
    def _num(text, default=0.0):
        try:
            return float(str(text).strip() or default)
        except (TypeError, ValueError):
            return default

    def _grid_paste(self, text):
        """粘贴：按 tab / 换行拆分，逐行追加（编号重复或为空的行跳过）。"""
        rows = []
        for ln in text.replace('\\r\\n', '\\n').replace('\\r', '\\n').split('\\n'):
            if ln.strip():
                cells = [c.strip() for c in ln.split('\\t')]
                cells += [''] * (8 - len(cells))
                rows.append(cells)
        if not rows:
            self.win.setText(L_EDIT_COUNT, '剪贴板里没有可粘贴的行')
            return
        have = set(x['code'] for x in self.nodes)
        added = skipped = 0
        for cells in rows:
            code = cells[0]
            if not code or code in have:
                skipped += 1
                continue
            have.add(code)
            self.nodes.append(dict(code=code, name=cells[1], upstream=cells[2], cable=cells[3],
                                   loadKw=self._num(cells[4]), voltage=cells[5] or '380',
                                   length=self._num(cells[6])))
            added += 1
        self.fill_table()
        if added:
            self.rebuild(fit=False)
        self.win.setText(L_EDIT_COUNT, '已粘贴 %d 行%s' % (added, ('，跳过 %d 行' % skipped) if skipped else ''))
        self.win.setText(L_STATUS, '已粘贴 %d 行，共 %d 行' % (added, len(self.nodes)))

    def _grid_sort(self, col):
        """点列头排序：按该列重排 nodes（数值列按数值，其余按文本）。"""
        if not (0 <= col < len(FIELDS)):
            return
        key = FIELDS[col]
        if key in ('loadKw', 'length'):
            def k(n):
                return float(n.get(key) or 0)
        else:
            def k(n):
                return str(n.get(key) or '')
        self.nodes = sorted(self.nodes, key=k)
        self.fill_table()
        self.rebuild(fit=False)
        self.win.setText(L_EDIT_COUNT, '已按「%s」排序' % COLS[col])

'''
t = t.replace(anchor, NEW, 1)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('Python 表格动作完成')
