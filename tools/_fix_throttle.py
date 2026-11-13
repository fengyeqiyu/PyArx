import io
p = 'G:/PyArx/tests/mfc_xaml/pp_app.py'
s = io.open(p, encoding='utf-8').read()
old = """        if phase == 'move' and (now - getattr(self, '_split_t', 0.0)) < 0.033:
            return"""
new = """        # 时间窗内合并，但位移超过 6px 必须立刻应用：否则快速拖动会被合并成
        # "几乎没动"，测试与手感都表现为分割条跟不上手。
        moved = abs(int(x) - getattr(self, '_split_last', -99999))
        if (phase == 'move' and moved < 6
                and (now - getattr(self, '_split_t', 0.0)) < 0.033):
            return
        self._split_last = int(x)"""
assert s.count(old) == 1
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('拖动合并：位移>=6px 立即应用')
