# -*- coding: utf-8 -*-
"""画布为什么没命中：查窗口区域（SetWindowRgn）、NCHITTEST、以及兄弟 Z 序。"""
import ctypes
import ctypes.wintypes as wt
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXaml as M
import pp_app
import pp_events as E

user32 = ctypes.WinDLL('user32', use_last_error=True)
gdi32 = ctypes.WinDLL('gdi32', use_last_error=True)
user32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
user32.WindowFromPoint.argtypes = [wt.POINT]
user32.WindowFromPoint.restype = wt.HWND
user32.ChildWindowFromPointEx.argtypes = [wt.HWND, wt.POINT, wt.UINT]
user32.ChildWindowFromPointEx.restype = wt.HWND
user32.RealChildWindowFromPoint.argtypes = [wt.HWND, wt.POINT]
user32.RealChildWindowFromPoint.restype = wt.HWND
user32.GetParent.argtypes = [wt.HWND]
user32.GetParent.restype = wt.HWND
user32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.SendMessageW.restype = ctypes.c_ssize_t
user32.ClientToScreen.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]
user32.ScreenToClient.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]
user32.GetWindowRgn.argtypes = [wt.HWND, wt.HRGN]
user32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                                ctypes.c_int, ctypes.c_int, wt.UINT]
user32.SetWindowLongPtrW.argtypes = [wt.HWND, ctypes.c_int, ctypes.c_ssize_t]
user32.SetWindowLongPtrW.restype = ctypes.c_ssize_t
user32.IsWindowVisible.argtypes = [wt.HWND]
gdi32.CreateRectRgn.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
gdi32.CreateRectRgn.restype = wt.HRGN
gdi32.GetRgnBox.argtypes = [wt.HRGN, ctypes.POINTER(wt.RECT)]
gdi32.DeleteObject.argtypes = [wt.HGDIOBJ]

WM_NCHITTEST = 0x0084


def cls_of(h):
    buf = ctypes.create_unicode_buffer(128)
    user32.GetClassNameW(h, buf, 128) if hasattr(user32, 'GetClassNameW') else None
    return buf.value + ' #%s' % user32.GetWindowLongPtrW(h, -12)


app = pp_app.App()
app.win.show()
M.pump(120)
app.showEditor()
M.pump(300)
hwnd = app.win.hwnd
user32.SetWindowPos(hwnd, wt.HWND(-1), 40, 30, 0, 0, 0x0001 | 0x0040)
M.pump(500)

cv = E.find_control(hwnd, pp_app.CV)
print('画布 = %s' % hex(cv), flush=True)

wr = wt.RECT()
user32.GetWindowRect(cv, ctypes.byref(wr))
rc = app.win.cardRectPlan(pp_app.CV, app.nodes[2]['code'])
pt = wt.POINT(int(rc[0] + rc[2] / 2), int(rc[1] + rc[3] / 2))
user32.ClientToScreen(cv, ctypes.byref(pt))
print('画布屏幕 (%d,%d)-(%d,%d)  测试点 (%d,%d)' % (wr.left, wr.top, wr.right, wr.bottom,
                                                pt.x, pt.y), flush=True)

# 1. 窗口区域
rgn = gdi32.CreateRectRgn(0, 0, 0, 0)
ok = user32.GetWindowRgn(cv, rgn)
box = wt.RECT()
gdi32.GetRgnBox(rgn, ctypes.byref(box))
rtype_names = {0: 'ERROR', 1: 'NULLREGION', 2: 'SIMPLEREGION', 3: 'COMPLEXREGION'}
print('GetWindowRgn=%s 类型=%s box=(%d,%d)-(%d,%d)'
      % (ok, rtype_names.get(ok, ok), box.left, box.top, box.right, box.bottom), flush=True)
gdi32.DeleteObject(rgn)

# 2. NCHITTEST（父窗与画布自己）
cpt = wt.POINT(pt.x, pt.y)
user32.ScreenToClient(cv, ctypes.byref(cpt))
print('客户区点 = (%d,%d)' % (cpt.x, cpt.y), flush=True)
r = user32.SendMessageW(cv, WM_NCHITTEST, 0,
                        (pt.y & 0xFFFF) << 16 | (pt.x & 0xFFFF))
print('画布 WM_NCHITTEST -> %s (HTCLIENT=1, HTTRANSPARENT=-1, HTNOWHERE=0)'
      % ctypes.c_short(r).value, flush=True)

# 3. 系统命中：主窗的子查找
print('WindowFromPoint      -> %s' % cls_of(user32.WindowFromPoint(pt)), flush=True)
p_child = wt.POINT(pt.x, pt.y)
user32.ScreenToClient(hwnd, ctypes.byref(p_child))
print('RealChildWindowFromPoint(主窗) -> %s'
      % cls_of(user32.RealChildWindowFromPoint(hwnd, p_child)), flush=True)
print('ChildWindowFromPointEx(主窗)  -> %s'
      % cls_of(user32.ChildWindowFromPointEx(hwnd, p_child, 0x0003)), flush=True)

# 4. 画布是否在 Z 序最上：把它提到顶再试
user32.SetWindowPos(cv, wt.HWND_TOP, 0, 0, 0, 0, 0x0001 | 0x0002 | 0x0010)  # NOSIZE|NOMOVE|NOACTIVATE
M.pump(120)
print('提到 Z 顶后 WindowFromPoint -> %s' % cls_of(user32.WindowFromPoint(pt)), flush=True)

os._exit(0)
