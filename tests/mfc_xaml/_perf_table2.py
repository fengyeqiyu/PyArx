# -*- coding: utf-8 -*-
"""表格交互耗时：重绘、点行、滚一格、滚动到底。"""
import os, sys, time, ctypes as C
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M
import pp_app, pp_events as E

u32 = C.windll.user32
app = pp_app.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(400)
tbl = E.find_control(app.win.hwnd, pp_app.TBL)
print('tbl hwnd=%s rows=%d' % (tbl, app.win.getTableRowCount(pp_app.TBL)), flush=True)

def timed(label, fn, n=5):
    fn(); M.pump(50)
    ts = []
    for _ in range(n):
        a = time.perf_counter(); fn(); b = time.perf_counter(); ts.append((b - a) * 1000)
    print('%-30s min %6.1f  med %6.1f  max %6.1f ms' % (label, min(ts), sorted(ts)[len(ts)//2], max(ts)), flush=True)

# 1) 强制整表重绘（走宿主皮肤层）
def repaint():
    u32.InvalidateRect(C.cVoidP(tbl), None, True)
    u32.UpdateWindow(C.cVoidP(tbl))
timed('强制表格重绘', repaint, 5)

# 2) 整窗重绘
def repaint_all():
    u32.InvalidateRect(C.cVoidP(app.win.hwnd), None, True)
    u32.UpdateWindow(C.cVoidP(app.win.hwnd))
timed('强制整窗重绘', repaint_all, 5)

# 3) 点某一行（含 Python 回调 + 画布选中）
rc = app.win.cellRect(pp_app.TBL, 5, 1)
cx, cy = rc[0] + rc[2] // 2, rc[1] + rc[3] // 2
timed('点击第 6 行', lambda: E.click(tbl, cx, cy, pump=0), 5)

# 4) 滚一格
timed('滚动一格', lambda: E.wheel(tbl, -120, cx, cy, pump=0), 5)

# 5) fillTable 单次
timed('fillTable', lambda: app.fillTable(), 3)

# 6) 表里 39 行滚到底再回来
def scroll_all():
    for _ in range(20):
        E.wheel(tbl, -120, cx, cy, pump=0)
timed('连滚 20 格', scroll_all, 3)

os._exit(0)
