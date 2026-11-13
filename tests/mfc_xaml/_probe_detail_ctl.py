# -*- coding: utf-8 -*-
"""详情弹出帧：逐个 HWND 重绘耗时排序；并拆分 fillDetail 各阶段。"""
import os, sys, time, ctypes as C
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import PyMfcXaml as M
import pp_app, pp_events as E

u32 = C.windll.user32
app = pp_app.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(500)
app.fillDetail(app.nodes[1]['code']); M.pump(300)

name = {getattr(ppApp, k): k for k in
        ['DT_PANEL','DT_TITLE','DT_TIP','D_SAVE','D_ADDCHILD','D_DEL',
         'DT_LV','DT_LL','DT_LLOAD','D_VOL','D_LEN','D_LOAD']}
for i in range(4):
    name[pp_app.DT_L0 + i] = 'DT_L%d' % i
    name[pp_app.D_F0 + i] = 'D_F%d' % i

rows = []
for cid in app._detailIds:
    hw = E.find_control(app.win.hwnd, cid)
    if not hw:
        rows.append((cid, name.get(cid, str(cid)), 0.0, 0.0, 0.0, 0)); continue
    ts = []
    for _ in range(5):
        u32.InvalidateRect(C.cVoidP(hw), None, True); u32.UpdateWindow(C.cVoidP(hw))
        a = time.perf_counter()
        u32.InvalidateRect(C.cVoidP(hw), None, True); u32.UpdateWindow(C.cVoidP(hw))
        ts.append((time.perf_counter() - a) * 1000)
    rows.append((cid, name.get(cid, str(cid)), min(ts), sorted(ts)[len(ts)//2], max(ts), len(ts)))

rows.sort(key=lambda r: -r[3])
print('%-12s %-10s %6s %6s %6s' % ('id', 'name', 'min', 'med', 'max'), flush=True)
for cid, nm, mn, md, mx, n in rows:
    print('%-12d %-10s %6.2f %6.2f %6.2f' % (cid, nm, mn, md, mx), flush=True)

# 整帧：fillDetail 5 次
tt = []
for _ in range(5):
    app.curSel = None
    a = time.perf_counter(); app.fillDetail(app.nodes[1]['code']); M.pump(30)
    tt.append((time.perf_counter() - a) * 1000)
print('fillDetail+pump(30) 5 次均值 %.1f ms' % (sum(tt)/len(tt)), flush=True)

a = time.perf_counter(); M.pump(30); print('空 pump(30) %.1f ms' % ((time.perf_counter()-a)*1000), flush=True)
os._exit(0)
