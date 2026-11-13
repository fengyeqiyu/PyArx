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

u = ctypes.WinDLL('user32', use_last_error=True)
u.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
u.SendMessageW.restype = wt.LPARAM
u.GetWindow.argtypes = [wt.HWND, wt.UINT]
u.GetWindow.restype = wt.HWND
u.GetWindowLongPtrW.argtypes = [wt.HWND, ctypes.c_int]
u.GetWindowLongPtrW.restype = ctypes.c_ssize_t
u.ClientToScreen.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]
u.SetFocus.argtypes = [wt.HWND]
u.GetFocus.restype = wt.HWND

WM_MOUSEWHEEL, WM_LBUTTONDOWN, WM_MOUSEMOVE, WM_LBUTTONUP = 0x020A, 0x0201, 0x0200, 0x0202

def find(host, want):
    child = u.GetWindow(host, 5)
    while child:
        if int(u.GetWindowLongPtrW(child, -12)) == want:
            return child
        got = find(child, want)
        if got:
            return got
        child = u.GetWindow(child, 2)
    return None

app = pp_app.App()
app.win.show()
time.sleep(0.7)
M.pump(120)
host = app.win.hwnd
cv = find(host, pp_app.CV)
print('host=%s cv=%s zoom=%.3f view=%s' % (host, cv, app.win.getPlanZoom(pp_app.CV), app.win.getPlanView(pp_app.CV)))

def wheel(hw, cx, cy, delta):
    pt = wt.POINT(cx, cy)
    u.ClientToScreen(hw, ctypes.byref(pt))
    u.SendMessageW(hw, WM_MOUSEWHEEL, (delta & 0xFFFF) << 16, (pt.x & 0xFFFF) | ((pt.y & 0xFFFF) << 16))

print('--- 1 直接发给画布 ---')
z0 = app.win.getPlanZoom(pp_app.CV)
wheel(cv, 400, 300, 120)
M.pump(30)
print('  zoom %.3f -> %.3f' % (z0, app.win.getPlanZoom(pp_app.CV)))

print('--- 2 拖动 ---')
v0 = app.win.getPlanView(pp_app.CV)
u.SetFocus(cv)
u.SendMessageW(cv, WM_LBUTTONDOWN, 1, (100 & 0xFFFF) | ((100 & 0xFFFF) << 16))
for i in (1, 2, 3):
    x, y = 100 + i * 20, 100 + i * 10
    u.SendMessageW(cv, WM_MOUSEMOVE, 1, (x & 0xFFFF) | ((y & 0xFFFF) << 16))
u.SendMessageW(cv, WM_LBUTTONUP, 0, 0)
M.pump(40)
print('  view %s -> %s' % (v0, app.win.getPlanView(pp_app.CV)))

print('--- 3 WM_SETCURSOR ---')
print('  返回', u.SendMessageW(cv, 0x0020, 0, 0))
