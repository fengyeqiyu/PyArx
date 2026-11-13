# -*- coding: utf-8 -*-
"""诊断 2：逐个按键确认 C++ 是否派发（Insert / F2 / Ctrl+C）。"""
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
user32.GetClassNameW.argtypes = [wt.HWND, ctypes.c_wchar_p, ctypes.c_int]

GW_CHILD, GW_HWNDNEXT, GWL_ID = 5, 2, -12

calls = []


def find_by_id(parent, cid, depth=0):
    h = user32.GetWindow(parent, GW_CHILD)
    while h:
        if user32.GetWindowLongPtrW(h, GWL_ID) == cid:
            return h
        sub = find_by_id(h, cid, depth + 1)
        if sub:
            return sub
        h = user32.GetWindow(h, GW_HWNDNEXT)
    return None


app = pp_app.App()
app.win.show()
M.pump(80)
app.showEditor()
M.pump(200)


def probe(action, row, col, text):
    calls.append((action, row, col))
    print('  回调:', action, row, col, flush=True)


app.win.onGridAction(probe)
lv = find_by_id(app.win.hwnd, pp_app.TBL)
buf = ctypes.create_unicode_buffer(64)
user32.GetClassNameW(lv, buf, 64)
print('lv=%s class=%s' % (lv, buf.value), flush=True)
print('表头句柄=',
      user32.SendMessageW(lv, 0x101F, 0, 0), flush=True)

app.win.setTableSelectedRow(pp_app.TBL, 2)
M.pump(60)

for name, vk in (('INSERT', 0x2D), ('F2', 0x71), ('DELETE', 0x2E)):
    print('[%s] 发送 WM_KEYDOWN 0x%02X' % (name, vk), flush=True)
    user32.SendMessageW(lv, 0x0100, vk, 0x00000001)
    M.pump(80)
    print('   calls=%s' % calls, flush=True)

os._exit(0)
