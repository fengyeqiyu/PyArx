import io
p = 'G:/PyArx/tests/mfc_xaml/pp_real_test.py'
s = io.open(p, encoding='utf-8').read()

old = """cv = E.find_control(hwnd, pp_app.CV)
rc = app.win.cardRectPlan(pp_app.CV, app.nodes[2]['code'])
pt = wt.POINT(int(rc[0] + rc[2] / 2), int(rc[1] + rc[3] / 2))
user32.ClientToScreen(cv, ctypes.byref(pt))
print('测试点 (%d,%d)' % (pt.x, pt.y), flush=True)"""
new = """cv = E.find_control(hwnd, pp_app.CV)
CODE = app.nodes[2]['code']


def card_screen_pt():
    \"\"\"每次现算卡片中心 → 屏幕坐标。窗口位置/尺寸会随系统摆放变化，
    算一次复用会让点击落到画布空白处（历史上 4/6 就是这个原因）。\"\"\"
    rc = app.win.cardRectPlan(pp_app.CV, CODE)
    p = wt.POINT(int(rc[0] + rc[2] / 2), int(rc[1] + rc[3] / 2))
    user32.ClientToScreen(cv, ctypes.byref(p))
    return p


# 先在客户坐标里确认这个点真的命中该卡片，命中才去点
p = card_screen_pt()
cx, cy = p.x, p.y
pt = p
print('测试点 (%d,%d) hit=%r' % (pt.x, pt.y, app.win.hitPlan(pp_app.CV, ), ), flush=True)"""
assert s.count(old) == 1
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('patched partial?')
