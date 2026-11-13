import io
P = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
t = io.open(P, encoding='utf-8-sig').read()

# 1) 初始化列宽缓存
old = """        self._detail_visible = False          # 详情浮层是否可见（_layout 要用它决定显隐）"""
new = """        self._detail_visible = False          # 详情浮层是否可见（_layout 要用它决定显隐）
        self._colw_now = None                 # 当前列宽（随侧板宽度变化重算）"""
assert t.count(old) == 1
t = t.replace(old, new)

# 2) 侧板：标题宽度 + 列宽自适应
t = t.replace("(L_EDIT_COUNT, px + 78, ed_y + 12, 100, 20, True),",
              "(L_EDIT_COUNT, px + 78, ed_y + 12, 160, 20, True),")

old2 = """        # 表格：占满剩余高度，超出部分由 ListView 自己的滚动条处理
        tbl_y = ed_y + ED_HEAD_H + 2"""
new2 = """        # 列宽按可用宽度等比缩放（末列 ✕ 固定 26），避免不该出现的横向滚动条
        widths = self._grid_columns(inner)
        if widths != self._colw_now and hasattr(self.win, 'setColumnWidth'):
            self._colw_now = widths
            for i, pxw in enumerate(widths):
                self.win.setColumnWidth(TBL, i, pxw)
        # 表格：占满剩余高度，超出部分由 ListView 自己的滚动条处理
        tbl_y = ed_y + ED_HEAD_H + 2"""
assert t.count(old2) == 1
t = t.replace(old2, new2)

# 3) 列宽计算函数
anchor = "    def on_resize(self, cw, ch):"
NEW = '''    @staticmethod
    def _grid_columns(inner):
        """按 HTML 的列宽比例铺满侧板（末列 ✕ 固定 26px）。"""
        fixed = 26
        base = float(sum(COLW[:7]))
        avail = max(240, inner - 22 - fixed)     # 22 ≈ 垂直滚动条 + 边框
        widths = [max(34, int(round(COLW[i] * avail / base))) for i in range(7)]
        return widths + [fixed]

'''
t = t.replace(anchor, NEW + anchor, 1)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('列宽自适应完成')
