# -*- coding: utf-8 -*-
"""逐控件重绘耗时：找出分项 paint 成本。"""
import os, sys, time, ctypes as C
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M
import pp_app, pp_events as E

u32 = C.windll.user32
app = pp_app.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(500)

ids = {'CV': pp_app.CV, 'TBL': pp_app.TBL, 'ED_PANEL': pp_app.ED_PANEL,
       'MINI': pp_app.MINI, 'ST_PANEL': pp_app.ST_PANEL}
hws = {k: E.find_control(app.win.hwnd, v) for k, v in ids.items()}

def timed(label, hw, n=5):
    if not hw:
        print('%-16s 未找到' % label, flush=True)
        return
    u32.InvalidateRect(C.c_void_p(hw), None, True)
    u32.UpdateWindow(C.c_void_p(hw))
    ts = []
    for _ in range(n):
        a = time.perf_counter()
        u32.InvalidateRect(C.c_void_p(hw), None, True)
        u32.UpdateWindow(C.c_void_p(hw))
        b = time.perf_counter()
        ts.append((b - a) * 1000)
    print('%-16s min %6.1f  med %6.1f  max %6.1f ms' % (
        label, min(ts), sorted(ts)[len(ts)//2], max(ts)), flush=True)

for k, hw in hws.items():
    timed(k, hw)

a = time.perf_counter(); M.pump(30); b = time.perf_counter()
print('空 pump(30)      %6.1f ms' % ((b - a) * 1000), flush=True)
os._exit(0)
