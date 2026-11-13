# -*- coding: utf-8 -*-
"""找出工具栏按钮的窗口句柄：按控件 id 递归枚举子窗口。"""
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
PROC = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
user32.EnumChildWindows.argtypes = [wt.HWND, PROC, wt.LPARAM]
user32.GetWindowLongW.argtypes = [wt.HWND, ctypes.c_int]
user32.GetWindowLongW.restype = ctypes.c_long
user32.GetClassNameW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
user32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]

found = []

app = pp_app.App()
app.win.show()
M.pump(250)
app.onDualSource()
M.pump(250)


def cb(h, l):
    cid = user32.GetWindowLongW(h, -12) & 0xFFFF
    cls = ctypes.create_unicode_buffer(128)
    user32.GetClassNameW(h, cls, 128)
    r = wt.RECT()
    user32.GetWindowRect(h, ctypes.byref(r))
    found.append((cid, h, cls.value, r.left, r.top, r.right - r.left, r.bottom - r.top))
    return True


user32.EnumChildWindows(app.win.hwnd, PROC(cb), 0)
print('子窗口数:', len(found), flush=True)
ids = {pp_app.B_EXLSX: '导出XLSX', pp_app.B_ECSV: '导出CSV', pp_app.B_XLSX: '导入XLSX',
       pp_app.B_DUAL: '双电源示例'}
for cid, h, cls, x, y, w, hh in found:
    if cid in ids:
        print('%s id=%d hwnd=%d class=%s rect=(%d,%d,%dx%d)' % (ids[cid], cid, h, cls, x, y, w, hh), flush=True)
print('id 命中数:', sum(1 for f in found if f[0] in ids), flush=True)
print('前 20 个:', [(f[0], f[2]) for f in found[:20]], flush=True)
