import io
P = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
s = io.open(P, encoding='utf-8').read()

# 1) 状态位
old = """        self._row_edit_open = False           # 行编辑条是否展开
        self._row_edit_row = -1               # 正在编辑的行号"""
new = """        self._row_edit_open = False           # 行编辑条是否展开（默认关，双击改走原地编辑）
        self._row_edit_row = -1               # 正在编辑的行号
        self._ce = None                       # 原地单元格编辑：(行, 列, 编辑框 id)
        self._tbl_pos = (0, 0)                # 表格左上角在侧板内的偏移"""
assert s.count(old) == 1
s = s.replace(old, new)

# 2) 布局时记下表格位置，供原地编辑换算坐标
old = """        body = max(row_h * 4, tbl_bottom - tbl_y - hdr_h)
        rows = max(4, body // row_h)
        tbl_h = hdr_h + rows * row_h
        items.append((TBL, px, tbl_y, inner, tbl_h, True))"""
new = """        body = max(row_h * 4, tbl_bottom - tbl_y - hdr_h)
        rows = max(4, body // row_h)
        tbl_h = hdr_h + rows * row_h
        self._tbl_pos = (px, tbl_y)          # 原地编辑按它把 EDIT 盖到单元格上
        items.append((TBL, px, tbl_y, inner, tbl_h, True))"""
assert s.count(old) == 1
s = s.replace(old, new)

# 3) 原地编辑实现（放在 on_row_cancel 之后）
old = """    def on_row_cancel(self):
        self._row_edit_close()"""
new = """    def on_row_cancel(self):
        self._row_edit_close()

    # ---------- 原地单元格编辑（双击某格 = 直接在那一格里改） ----------
    def _cell_begin(self, row, col):
        \"\"\"双击单元格：把该列的 EDIT 精确盖到这一格上，聚焦并全选，就是 Excel 的手感。\"\"\"
        w = self.win
        n = len(self.nodes)
        if not (0 <= row < n) or not (0 <= int(col) < 7):
            return
        col = int(col)
        if not hasattr(w, 'cellRect'):
            self._open_row_edit(row)          # 宿主不带该能力时退回底部编辑条
            return
        self._cell_end(commit=True)           # 先把上一格结掉
        rc = w.cellRect(TBL, row, col)
        if not rc:
            return
        cx, cy, cw, ch = rc
        tx, ty = self._tbl_pos
        eid = E_F0 + col
        self._ce = (row, col, eid)
        w.moveMany([(eid, tx + cx, ty + cy, max(40, cw), max(20, ch), True)])
        w.setText(eid, w.getTableCell(TBL, row, col) or '')
        w.setVisible(eid, True)
        w.raiseMany([eid])
        w.setFocus(eid)
        w.setText(L_EDIT_COUNT, '正在编辑第 %d 行的「%s」' % (row + 1, COLS[col]))

    def _cell_blur(self, *a):
        \"\"\"EDIT 失焦 = 提交（点别处/点另一个格都算确认）。\"\"\"
        self._cell_end(commit=True)

    def _cell_end(self, commit=True):
        ce = self._ce
        if not ce:
            return
        self._ce = None                       # 先清，避免 setVisible 触发失焦再进一遍
        row, col, eid = ce
        txt = (self.win.getText(eid) or '').strip()
        self.win.setVisible(eid, False)
        if commit:
            self._cell_apply(row, col, txt)

    def _cell_apply(self, row, col, txt):
        \"\"\"单格写回：先按这一列的口径校验，再落库、刷新该格、重绘。\"\"\"
        w = self.win
        n = len(self.nodes)
        if not (0 <= row < n):
            return
        nd = self.nodes[row]
        key = FIELDS[col]
        line = '第 %d 行：' % (row + 1)
        if key == 'code':
            if not txt:
                w.setText(L_EDIT_COUNT, line + '编号不能为空')
                return
            if any(o.get('code') == txt for i, o in enumerate(self.nodes) if i != row):
                w.setText(L_EDIT_COUNT, line + '编号「%s」已存在' % txt)
                return
        elif key in ('loadKw', 'length'):
            try:
                fv = float(txt or 0)
            except ValueError:
                w.setText(L_EDIT_COUNT, line + ('负载' if key == 'loadKw' else '长度')
                          + '必须是不小于 0 的数字')
                return
            if fv < 0:
                w.setText(L_EDIT_COUNT, line + ('负载' if key == 'loadKw' else '长度')
                          + '必须是不小于 0 的数字')
                return
            txt = str(int(fv)) if fv == int(fv) else ('%g' % fv)
        elif key == 'upstream':
            if txt and txt == nd.get('code'):
                w.setText(L_EDIT_COUNT, line + '上游不能是节点自己')
                return

        old_code = nd.get('code') or ''
        if key == 'loadKw' or key == 'length':
            nd[key] = float(txt or 0)
        else:
            nd[key] = txt
        if key == 'code' and old_code != txt:
            # 改编号要顺带把下游的"上游"引用换掉，否则连线断
            for o in self.nodes:
                if (o.get('upstream') or '') == old_code:
                    o['upstream'] = txt
            self.collapsed.discard(old_code)
            if self.cur_sel == old_code:
                self.cur_sel = txt
            self.fill_table()                 # 编号列与上游列都要跟着变
        else:
            w.setTableCell(TBL, row, col, self._grid_text(nd, key))
        self.rebuild(fit=False)
        if self.cur_sel == nd.get('code'):
            self.fill_detail(nd.get('code'))
        w.setText(L_EDIT_COUNT, '第 %d 行的「%s」已更新' % (row + 1, COLS[col]))"""
assert s.count(old) == 1
s = s.replace(old, new)

# 4) on_grid_action：双击原地编辑 / cellend 提交 / F2 也走原地
old = """        if action == 'edit':
            if 0 <= r < n:
                self._open_row_edit(r)"""
new = """        if action == 'dbl':
            if 0 <= row < n:
                self._cell_begin(row, int(col))
        elif action == 'cellend':
            # row = 编辑框 id，col = 按键（13 回车确认 / 27 Esc 取消）
            self._cell_end(commit=(int(col) != 27))
        elif action == 'edit':
            if 0 <= r < n:
                self._cell_begin(r, int(col or 0))"""
assert s.count(old) == 1
s = s.replace(old, new)

# 5) 给 7 个编辑框绑失焦提交
old = """        w.onDoubleClick(TBL, self.on_grid_dbl)"""
new = """        w.onDoubleClick(TBL, self.on_grid_dbl)
        if hasattr(w, 'onBlur'):
            for i in range(7):
                w.onBlur(E_F0 + i, self._cell_blur)"""
assert s.count(old) == 1
s = s.replace(old, new)

io.open(P, 'w', encoding='utf-8').write(s)
print('Python：原地单元格编辑已实现')
