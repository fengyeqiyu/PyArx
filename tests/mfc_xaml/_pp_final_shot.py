# -*- coding: utf-8 -*-
"""最终态截图 + 画布交互复测（新布局下画布铺满内容区，手型/滚轮/拖动应全部可用）。"""
import ctypes
import ctypes.wintypes as wt
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXaml as M
import pp_app

user32 = ctypes.WinDLL('user32', use_last_error=True)
user32.GetWindow.argtypes = [wt.HWND, wt.UINT]
user32.GetWindow.restype = wt.HWND
user32.GetWindowLongPtrW.argtypes = [wt.HWND, ctypes.c_int]
user32.GetWindowLongPtrW.restype = ctypes.c_ssize_t
user32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.SendMessageW.restype = ctypes.c_ssize_t
user32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
user32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                                ctypes.c_int, ctypes.c_int, wt.UINT]

GW_CHILD, GW_HWNDNEXT, GWL_ID = 5, 2, -12
WM_MOUSEWHEEL, WM_LBUTTONDOWN, WM_MOUSEMOVE, WM_LBUTTONUP = 0x020A, 0x0201, 0x0200, 0x0202
WM_SETCURSOR = 0x0020


def find_by_id(parent, cid):
    h = user32.GetWindow(parent, GW_CHILD)
    while h:
        if user32.GetWindowLongPtrW(h, GWL_ID) == cid:
            return h
        sub = find_by_id(h, cid)
        if sub:
            return sub
        h = user32.GetWindow(h, GW_HWNDNEXT)
    return None


app = pp_app.App()
app.win.show()
M.pump(80)
app.showEditor()
M.pump(250)

hwnd = app.win.hwnd
cv = find_by_id(hwnd, pp_app.CV)
r = wt.RECT()
user32.GetWindowRect(cv, ctypes.byref(r))
print('画布 HWND=%s 屏幕矩形 %dx%d @(%d,%d)'
      % (cv, r.right - r.left, r.bottom - r.top, r.left, r.top), flush=True)

z0 = app.win.getPlanZoom(pp_app.CV)
view0 = app.win.getPlanView(pp_app.CV)
# 滚轮一档（光标在画布内 200,200）
user32.SendMessageW(cv, WM_MOUSEWHEEL, (120 << 16), (200) | (200 << 16))
M.pump(60)
z1 = app.win.getPlanZoom(pp_app.CV)
print('滚轮 %s -> %s' % (z0, z1), flush=True)

# 左键拖动
user32.SendMessageW(cv, WM_LBUTTONDOWN, 1, (300) | (300 << 16))
M.pump(30)
user32.SendMessageW(cv, WM_MOUSEMOVE, 1, (360) | (340 << 16))
M.pump(30)
user32.SendMessageW(cv, WM_MOUSEMOVE, 1, (400) | (380 << 16))
M.pump(30)
user32.SendMessageW(cv, WM_LBUTTONUP, 0, (400) | (380 << 16))
M.pump(60)
view1 = app.win.getPlanView(pp_app.CV)
print('拖动 view %s -> %s' % (view0, view1), flush=True)

# 光标：WM_SETCURSOR 返回 1 = 控件自己设了光标
cur = user32.SendMessageW(cv, WM_SETCURSOR, cv, 0x0001 | (0x0200 << 16))
print('WM_SETCURSOR 返回 %s' % cur, flush=True)

# 复位视图再截图
app.fitView()
M.pump(120)
user32.SetWindowPos(hwnd, wt.HWND(-1), 40, 30, 0, 0, 0x0001 | 0x0040)
M.pump(500)
app.win.savePng(r'G:\PyArx\docs\_pp_lv_final.png')
M.pump(150)
print('截图完成', flush=True)
os._exit(0)
