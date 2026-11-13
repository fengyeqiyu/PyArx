import io
p = 'G:/PyArx/tests/mfc_xaml/pp_app.py'
s = io.open(p, encoding='utf-8').read()
old = '''    def fill_table(self):
        """把 nodes 全量灌进原生 ListView。滚动、行高、列头都由控件自己管。"""
        w = self.win
        w.clearTableRows(TBL)
        for node in self.nodes:
            vals = [self._grid_text(node, k) for k in FIELDS] + [u'\\u2715']
            w.addTableRow(TBL, u'\\t'.join(vals))
        self._grid_rows = len(self.nodes)'''
new = '''    def fill_table(self):
        """把 nodes 全量灌进原生 ListView。滚动、行高、列头都由控件自己管。

        一次灌满整表（`setTableRows`）而不是逐行 `addTableRow`：后者每插一行都在
        控件重绘开启状态下失效一次，39 行实测 22.5 ms；整表一次插入只重画一遍。
        """
        w = self.win
        lines = []
        for node in self.nodes:
            vals = [self._grid_text(node, k) for k in FIELDS] + [u'\\u2715']
            lines.append(u'\\t'.join(vals))
        w.setTableRows(TBL, u'\\n'.join(lines))
        self._grid_rows = len(self.nodes)'''
assert s.count(old) == 1
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('fill_table 改批量灌入')
