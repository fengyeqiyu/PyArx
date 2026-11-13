import io
P = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
s = io.open(P, encoding='utf-8').read()

old = """        # 行编辑条：7 个输入按 HTML 的列宽排，下面一行 确定/删除/取消
        x = px
        for i in range(7):
            items.append((E_F0 + i, x, strip_y, COLW[i] - 2, 26, on))
            x += COLW[i]
        items += [(B_LOADROW, px, strip_y + 30, 72, 26, on),
                  (B_APPLYROW, px + 76, strip_y + 30, 72, 26, on),
                  (B_DELROW, px + 152, strip_y + 30, 72, 26, on)]
        # 列宽按可用宽度等比缩放（末列 ✕ 固定 26），避免不该出现的横向滚动条
        widths = self._grid_columns(inner)
        if widths != self._colw_now and hasattr(self.win, 'setColumnWidth'):
            self._colw_now = widths
            for i, pxw in enumerate(widths):
                self.win.setColumnWidth(TBL, i, pxw)
        # 表格：占满剩余高度，超出部分由 ListView 自己的滚动条处理
        tbl_y = ed_y + ED_HEAD_H + 2
        # 编辑条没展开时表格一直铺到 footer 上方；展开时给它让出两行
        tbl_bottom = (strip_y - 8) if on else (foot_y - 10)
        tbl_h = max(120, tbl_bottom - tbl_y)
        items.append((TBL, px, tbl_y, inner, tbl_h, True))
        return items"""
new = """        # 列宽按可用宽度等比缩放（末列 ✕ 固定 26），避免不该出现的横向滚动条
        widths = self._grid_columns(inner)
        if widths != self._colw_now and hasattr(self.win, 'setColumnWidth'):
            self._colw_now = widths
            for i, pxw in enumerate(widths):
                self.win.setColumnWidth(TBL, i, pxw)
        # 行编辑条：7 个输入框必须用"缩放后的列宽"排，才能和表格的列一一对齐；
        # 用原始 COLW 排会让输入框边界和列边界错开，看起来就是"乱套"。
        x = px
        for i in range(7):
            items.append((E_F0 + i, x, strip_y, widths[i] - 2, 26, on))
            x += widths[i]
        items += [(B_LOADROW, px, strip_y + 30, 72, 26, on),
                  (B_APPLYROW, px + 76, strip_y + 30, 72, 26, on),
                  (B_DELROW, px + 152, strip_y + 30, 72, 26, on)]
        # 表格：高度按"表头 + 整数行"对齐。不对齐的话最后一行只露半截，
        # 选中行会在编辑条上方留一条蓝边，看着像表格残留。
        tbl_y = ed_y + ED_HEAD_H + 2
        tbl_bottom = (strip_y - 8) if on else (foot_y - 10)
        row_h, hdr_h = 21, 24
        if hasattr(self.win, 'getTableMetrics'):
            try:
                m = self.win.getTableMetrics(TBL)
                if m and m[0] > 0:
                    row_h, hdr_h = int(m[0]), int(m[1])
            except Exception:
                pass
        body = max(row_h * 4, tbl_bottom - tbl_y - hdr_h)
        rows = max(4, body // row_h)
        tbl_h = hdr_h + rows * row_h
        items.append((TBL, px, tbl_y, inner, tbl_h, True))
        return items"""
assert s.count(old) == 1
s = s.replace(old, new)
io.open(P, 'w', encoding='utf-8').write(s)
print('Python：编辑条按缩放后列宽对齐 + 表格整行对齐')
