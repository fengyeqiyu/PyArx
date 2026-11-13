# -*- coding: utf-8 -*-
import ctypes, ctypes.wintypes as wt, os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
sys.path.insert(0, r'G:\PyArx\pylibs')
import numpy as np
import PyMfcXaml as M
import pp_app

u32 = ctypes.WinDLL('user32')
GW_CHILD, GW_HWNDNEXT, GWL_ID = 5, 2, -12
app = pp_app.App(); app.win.show(); M.pump(120); app.showEditor(); M.pump(300)
h = app.win.hwnd
wr = wt.RECT(); u32.GetWindowRect(h, ctypes.byref(wr))

off = np.load(r'G:\PyArx\docs\_s2b_0_initial.npy')
on = np.load(r'G:\PyArx\docs\_s2b_1_initial.npy')
diff = np.abs(on.astype(np.int16) - off.astype(np.int16))
mask = (diff[:, :, 0] > 10) | (diff[:, :, 1] > 10) | (diff[:, :, 2] > 10)

def walk(ch):
    out = []
    while ch:
        cid = u32.GetWindowLongPtrW(ch, GWL_ID)
        r = wt.RECT(); u32.GetWindowRect(ch, ctypes.byref(r))
        out.append((cid, ch, r))
        sub = u32.GetWindow(ch, GW_CHILD)
        out += walk(sub)
        ch = u32.GetWindow(ch, GW_HWNDNEXT)
    return out

print('win', wr.left, wr.top, wr.right, wr.bottom, 'total mask', int(mask.sum()))
for cid, ch, r in walk(u32.GetWindow(h, GW_CHILD)):
    x0 = r.left - wr.left; y0 = r.top - wr.top
    x1 = r.right - wr.left; y1 = r.bottom - wr.top
    if x1 <= x0 or y1 <= y0: continue
    sub = mask[y0:y1, x0:x1]
    if sub.size == 0: continue
    n = int(sub.sum())
    if n:
        print('id=%4d rect=(%d,%d,%d,%d) mask=%d ratio=%.3f' % (cid, x0, y0, x1, y1, n, n / sub.size))
