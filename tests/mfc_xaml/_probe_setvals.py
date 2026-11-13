# -*- coding: utf-8 -*-
"""详情写值逐调用计时：setText / combo 字符串 / setCurSel，看哪条消息最贵。"""
import os, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import PyMfcXaml as M
import pp_app

app = pp_app.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(400)
w = app.win
n = app.nodeOf(app.nodes[1]['code'])
code = n['code']

calls = [
    ('DT_TITLE setText', lambda: w.setText(pp_app.DT_TITLE, '节点 %s' % code)),
    ('F0 setText', lambda: w.setText(pp_app.D_F0 + 0, n['code'])),
    ('F1 setText', lambda: w.setText(pp_app.D_F0 + 1, n.get('name') or '')),
    ('F2 setText', lambda: w.setText(pp_app.D_F0 + 2, n.get('upstream') or '')),
    ('F3 setText', lambda: w.setText(pp_app.D_F0 + 3, n.get('cable') or '')),
    ('VOL clearStrings', lambda: w.clearStrings(pp_app.D_VOL)),
]
for o in [('380', '380V 三相'), ('220', '220V 单相'), ('10kV', '10kV 中压'),
          ('35kV', '35kV 中压'), ('110kV', '110kV 高压')]:
    calls.append(('VOL addString', lambda o=o: w.addString(pp_app.D_VOL, o[1])))
calls += [
    ('VOL setCurSel', lambda: w.setCurSel(pp_app.D_VOL, 0)),
    ('LEN setText', lambda: w.setText(pp_app.D_LEN, '12.5')),
    ('LOAD setText', lambda: w.setText(pp_app.D_LOAD, '12.5')),
    ('TIP setText', lambda: w.setText(pp_app.DT_TIP, 'tip')),
]

# 预热一遍
for _, f in calls: f()
M.pump(20)

for name, f in calls:
    ts = []
    for _ in range(10):
        t = time.perf_counter(); f(); ts.append((time.perf_counter() - t) * 1000.0)
    ts.sort()
    print('%-20s min %.2f med %.2f max %.2f' % (name, ts[0], ts[5], ts[-1]), flush=True)

print('OK', flush=True)
os._exit(0)
