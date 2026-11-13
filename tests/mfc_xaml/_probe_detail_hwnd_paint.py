# -*- coding: utf-8 -*-
"""逐个测量详情 21 个 HWND 的同步重绘耗时，按耗时排序。"""
import os, sys, time, ctypes
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import PyMfcXaml as M
import pp_app
import pp_events as E

u = ctypes.windll.user32
u.InvalidateRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int]
u.UpdateWindow.argtypes = [ctypes.c_void_p]
u.IsWindowVisible.argtypes = [ctypes.c_void_p]

app = pp_app.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(300)
app.fillDetail(app.nodes[0]['code']); M.pump(200)

names = {pp_app.DT_PANEL: 'DT_PANEL', pp_app.DT_TITLE: 'DT_TITLE', pp_app.DT_TIP: 'DT_TIP',
         pp_app.DT_L0 + 0: 'DT_L0(编号)', pp_app.DT_L0 + 1: 'DT_L0(名称)',
         pp_app.DT_L0 + 2: 'DT_L0(上游)', pp_app.DT_L0 + 3: 'DT_L0(电缆)',
         pp_app.DT_LV: 'DT_LV', pp_app.DT_LL: 'DT_LL', pp_app.DT_LLOAD: 'DT_LLOAD',
         pp_app.D_F0 + 0: 'D_CODE', pp_app.D_F0 + 1: 'D_NAME',
         pp_app.D_F0 + 2: 'D_UP', pp_app.D_F0 + 3: 'D_CABLE',
         pp_app.D_VOL: 'D_VOL', pp_app.D_LEN: 'D_LEN', pp_app.D_LOAD: 'D_LOAD',
         pp_app.D_SAVE: 'D_SAVE', pp_app.D_ADDCHILD: 'D_ADDCHILD', pp_app.D_DEL: 'D_DEL'}

rows = []
for cid, label in names.items():
    hw = E.find_control(app.win.hwnd, cid)
    if not hw:
        rows.append((9999, label, cid, 0, 0, 0, 0)); continue
    if not u.IsWindowVisible(hw):
        rows.append((9998, label, cid, hw, -1, -1, -1)); continue
    ts = []
    for _ in range(5):
        u.InvalidateRect(hw, None, 1); u.UpdateWindow(hw)
        t0 = time.perf_counter()
        u.InvalidateRect(hw, None, 1); u.UpdateWindow(hw)
        ts.append((time.perf_counter() - t0) * 1000)
    rows.append((sorted(ts)[len(ts)//2], label, cid, hw, min(ts), sorted(ts)[len(ts)//2], max(ts)))

rows.sort(reverse=True)
print('%-14s %-5s %-12s %-10s %8s %8s %8s' % ('id', 'hid', '控件', 'visible', 'min', 'med', 'max'))
for med, label, cid, hw, mn, med2, mx in rows:
    print('%-14s %-5d %-12s %-10s %8.2f %8.2f %8.2f' % (
        cid, hw, label, mn != -1, mn if mn >= 0 else '-', med2 if med2 >= 0 else '-', mx if mx >= 0 else '-'))
print('同步重绘合计 med ≈ %.2f ms' % sum(r[0] for r in rows if r[0] < 9000))
os._exit(0)
