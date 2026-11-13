import io
P = r'G:\PyArx\tests\mfc_xaml\pp_real_test.py'
s = io.open(P, encoding='utf-8').read()
s = s.replace("""orig = wt.POINT()
user32.GetCursorPos(ctypes.byref(orig))""",
"""orig = wt.POINT()
user32.GetCursorPos(ctypes.byref(orig))

user32.GetForegroundWindow.restype = wt.HWND
user32.SetForegroundWindow.argtypes = [wt.HWND]
user32.SetForegroundWindow.restype = wt.BOOL
fg = user32.GetForegroundWindow()
print('前台窗口 = %s  目标 = %s' % (hex(fg or 0), hex(hwnd)), flush=True)
user32.SetForegroundWindow(hwnd)
M.pump(300)
fg2 = user32.GetForegroundWindow()
print('激活后前台 = %s' % hex(fg2 or 0), flush=True)""")
s = s.replace("""real_click(pt.x, pt.y)
sel = app.win.getPlanSelection(pp_app.CV)""",
"""real_click(pt.x, pt.y)
# 第一次点击可能只用于激活窗口，连点两下再判定
real_click(pt.x, pt.y)
sel = app.win.getPlanSelection(pp_app.CV)""")
io.open(P,'w',encoding='utf-8').write(s)
print('ok')
