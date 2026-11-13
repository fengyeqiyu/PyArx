# -*- coding: utf-8 -*-
"""扫全窗所有可见控件：哪些在 WM_NCHITTEST 上声明了"透明"（HTTRANSPARENT）。
自绘控件若回 -1，真实鼠标会穿透到父窗 —— 这就是点不中/滚不动的根源。"""
import ctypes
import ctypes.wintypes as wt
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')

import PyMfcXaml as M
import pp_app

user32 = ctypes.WinDLL('user32', use_last_error=True)
user32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, ctypes.c_void_p]
user32.SendMessageW.restype = ctypes.c_ssize_t
user32.GetClientRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
user32.ClientToScreen.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]
user32.WindowFromPoint.argtypes = [wt.POINT]
user32.WindowFromPoint.restype = wt.HWND
user32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
user32.IsWindowVisible.argtypes = [wt.HWND]
user32.GetClassNameW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
user32.GetWindowLongPtrW.argtypes = [wt.HWND, ctypes.c_int]
user32.GetWindowLongPtrW.restype = ctypes.c_ssize_t
user32.GetWindow.argtypes = [wt.HWND, wt.UINT]
user32.GetWindow.restype = wt.HWND
user32.GetParent.argtypes = [wt.HWND]
user32.GetParent.restype = wt.HWND

WM_NCHITTEST = 0x0084
GW_CHILD, GW_HWNDNEXT = 5, 2
GWL_ID = -12

HT = {-1: 'HTTRANSPARENT', 0: 'HTNOWHERE', 1: 'HTCLIENT', 2: 'HTCAPTION'}


def kids(h):
    out = []
    c = user32.GetWindow(h, GW_CHILD)
    while c:
        out.append(c)
        c = user32.GetWindow(c, GW_HWNDNEXT)
    return out


def walk(h, depth=0, acc=None):
    acc = [] if acc is None else acc
    for c in kids(h):
        acc.append((c, depth))
        walk(c, depth + 1, acc)
    return acc


app = pp_app.App()
app.win.show()
M.pump(200)
app.showEditor()
M.pump(400)

allc = walk(app.win.hwnd)
print('子控件总数 = %d' % len(allc), flush=True)

bad, ok, zero = [], 0, 0
for h, d in allc:
    if not user32.IsWindowVisible(h):
        continue
    rc = wt.RECT()
    user32.GetClientRect(h, ctypes.byref(rc))
    if rc.right < 3 or rc.bottom < 3:
        zero += 1
        continue
    pt = wt.POINT(rc.right // 2, rc.bottom // 2)
    user32.ClientToScreen(h, ctypes.byref(pt))
    hit = user32.SendMessageW(h, WM_NCHITTEST, 0,
                              ctypes.c_void_p((pt.y << 16) | (pt.x & 0xFFFF)))
    if hit == -1:
        cls = ctypes.create_unicode_buffer(64)
        user32.GetClassNameW(h, cls, 64)
        bad.append((user32.GetWindowLongPtrW(h, GWL_ID), cls.value, d, pt.x, pt.y))

if bad:
    print('!! 以下 %d 个控件声明了 HTTRANSPARENT（真实鼠标会穿透）:' % len(bad), flush=True)
    for i, c, d, x, y in bad:
        print('   id=%-6s cls=%-22s depth=%d 中心=(%d,%d)' % (i, c, d, x, y), flush=True)
else:
    print('全部可见控件均声明可点（HTCLIENT/normal），无穿透', flush=True)

print('=== 扫描完成：透明 %d 个 / 可点 %d 个 / 尺寸过小跳过 %d 个 ==='
      % (len(bad), ok, zero), flush=True)
os._exit(0)
