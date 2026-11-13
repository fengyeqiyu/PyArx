import io
P = r'G:\PyArx\tests\mfc_xaml\pp_real_test.py'
s = io.open(P, encoding='utf-8').read()
old = """chk('真实鼠标单击选中卡片', sel == app.nodes[2]['code'], repr(sel))"""
new = """print('探针 (lastMsg, upX, upY, wasDrag, moved, lastHit) = %s'
      % (app.win.planProbe(pp_app.CV),), flush=True)
chk('真实鼠标单击选中卡片', sel == app.nodes[2]['code'], repr(sel))"""
assert s.count(old) == 1
s = s.replace(old, new)
io.open(P,'w',encoding='utf-8').write(s)
print('探针读取已加')
