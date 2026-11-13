# -*- coding: utf-8 -*-
"""枚举宿主窗口的子窗口树，找画布真实 HWND（id = CV）。"""
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
u.GetWindow.argtypes = [wt.HWND, wt.UINT]
u.GetWindow.restype = wt.HWND
u.GetClassNameW.argtypes = [wt.HWND, ctypes.c_wchar_p, ctypes.c_int]
u.GetWindowLongPtrW.argtypes = [wt.HWND, ctypes.c_int]
u.GetWindowLongPtrW.restype = ctypes.c_ssize_t
ENUMPROC = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)


def cls(h):
    b = ctypes.create_unicode_buffer(128)
    u.GetClassNameW(h, b, 128)
    return b.value


def walk(h, depth=0, out=None):
    if out is None:
        out = []
    child = u.GetWindow(h, 5)          # GW_CHILD
    while child:
        gid = int(u.GetWindowLongPtrW(child, -12))                 # GWL_ID
        out.append((depth, child, gid, cls(child)))
        walk(child, depth + 1, out)
        child = u.GetWindow(child, 2)  # GW_HWNDNEXT
    return out


app = pp_app.App()
app.win.show()
time.sleep(0.7)
M.pump(120)
host = app.win.hwnd
target = pp_app.CV
print('host=%s CV id=%d' % (host, target))
for depth, h, gid, c in walk(host):
    mark = '  <== 画布' if gid == target else ''
    if depth <= 2 or gid == target:
        print('%s%s id=%-5d %s%s' % ('  ' * depth, h, gid, c, mark))

u.SetFocus.argtypes = [wt.HWND]
found = [h for d, h, g, c in walk(host) if g == target]
if found:
    cvh = found[0]
    print('画布 HWND =', cvh, 'class =', cls(cvh))
    u.SetFocus.argtypes = [wt.HWND]
    u.SetFocus(cvh)
    M.pump(20)
    print('设焦点后 GetFocus =', u.GetFocus())
