# -*- coding: utf-8 -*-
"""最小复现：一次向下、一次向上，打印侧板起点，确认 delta 方向处理。"""
import ctypes
import ctypes.wintypes as wt
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXaml as M
import pp_app

user32 = ctypes.WinDLL('user32', use_last_error=True)
user32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.SendMessageW.restype = wt.LPARAM
user32.ClientToScreen.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]


def wheel(hwnd, cx, cy, delta):
    pt = wt.POINT(cx, cy)
    user32.ClientToScreen(hwnd, ctypes.byref(pt))
    wparam = (delta & 0xFFFF) << 16
    lparam = (pt.x & 0xFFFF) | ((pt.y & 0xFFFF) << 16)
    print('  发 delta=%d wparam=0x%08X lparam=0x%08X' % (delta, wparam, lparam))
    user32.SendMessageW(hwnd, 0x020A, wparam, lparam)


app = pp_app.App()
app.win.show()
time.sleep(0.6)
M.pump(40)
app.showEditor()
M.pump(80)
hwnd = app.win.hwnd
rx, ry, w, h = app._grid_body_rect()
print('行区', (rx, ry, w, h), '起点', app._scroll_row)

for d in (-120, -120, 120, 120, 120):
    wheel(hwnd, rx + 150, ry + 150, d)
    M.pump(20)
    print('   -> 起点', app._scroll_row)
