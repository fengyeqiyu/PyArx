# -*- coding: utf-8 -*-
"""S2-A 三态整窗 PNG 取证（=1 表面路径）。"""
import ctypes
import ctypes.wintypes as wt
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
sys.path.insert(0, r'G:\PyArx\pylibs')

import PyMfcXaml as M
import pp_app
import pp_events as E

u32 = ctypes.WinDLL('user32', use_last_error=True)
u32.GetWindowLongPtrW.argtypes = [wt.HWND, ctypes.c_int]
u32.GetWindowLongPtrW.restype = ctypes.c_ssize_t
u32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
u32.SendMessageW.restype = ctypes.c_ssize_t

WM_LBUTTONDOWN, WM_MOUSEMOVE, WM_LBUTTONUP = 0x0201, 0x0200, 0x0202
MK_LBUTTON = 0x0001
GW_CHILD, GW_HWNDNEXT, GWL_ID = 5, 3, -12

OUT = r'G:\PyArx\docs'


def find_by_id(parent, cid):
    h = u32.GetWindow(parent, GW_CHILD)
    while h:
        if u32.GetWindowLongPtrW(h, GWL_ID) == cid:
            return h
        sub = find_by_id(h, cid)
        if sub:
            return sub
        h = u32.GetWindow(h, GW_HWNDNEXT)
    return None


app = pp_app.App()
app.win.show()
M.pump(120)
app.showEditor()
M.pump(300)

# 1. 初始
app.win.savePng(os.path.join(OUT, r'_s2a_initial.png'))
print('saved _s2a_initial.png', flush=True)

# 2. 弹详情（先于拖动，避免侧板收窄影响卡片命中）
cv = find_by_id(app.win.hwnd, pp_app.CV)
code = app.nodes[2]['code']
rc = app.win.cardRectPlan(pp_app.CV, code)
cx, cy = int(rc[0] + rc[2] / 2), int(rc[1] + rc[3] / 2)
E.click(cv, cx, cy, pump=120)
M.pump(120)
print('detail visible=%s code=%s' % (app._detailVisible, code), flush=True)
app.win.savePng(os.path.join(OUT, r'_s2a_detail.png'))
print('saved _s2a_detail.png', flush=True)

# 3. 拖分割条一步
spl = find_by_id(app.win.hwnd, pp_app.SPLIT)
cw, _ = app.clientSize()
w0 = app._sideWidth(cw)
u32.SendMessageW(spl, WM_LBUTTONDOWN, MK_LBUTTON, 0)
M.pump(30)
app.onSplitDrag('move', cw - w0 - pp_app.SPLIT_W + 40)
M.pump(120)
u32.SendMessageW(spl, WM_LBUTTONUP, 0, 0)
M.pump(60)
app.win.savePng(os.path.join(OUT, r'_s2a_split.png'))
print('saved _s2a_split.png', flush=True)

os._exit(0)
