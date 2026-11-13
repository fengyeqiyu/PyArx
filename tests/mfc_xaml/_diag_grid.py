# -*- coding: utf-8 -*-
"""诊断：F2 直达链路（selected 行 / Python 回调 / C++ 按键派发）。"""
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
user32.PostMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.PostMessageW.restype = wt.BOOL
user32.SetForegroundWindow.argtypes = [wt.HWND]
user32.SetFocus.argtypes = [wt.HWND]

GW_CHILD, GW_HWNDNEXT, GWL_ID = 5, 2, -12


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
M.pump(200)

hwnd = app.win.hwnd
lv = find_by_id(hwnd, pp_app.TBL)
print('lv=%s' % lv, flush=True)

app.win.setTableSelectedRow(pp_app.TBL, 3)
M.pump(80)
print('selected(after set) =', app.win.getTableSelectedRow(pp_app.TBL), flush=True)

# A) Python 侧直接走 edit 回调
try:
    app.onGridAction('edit', 3, 0, '')
    print('A python edit -> open=%s row=%s' % (app._rowEditOpen, app._rowEditRow), flush=True)
except Exception as e:
    import traceback
    print('A 异常:', e, flush=True)
    traceback.print_exc()

app._rowEditClose()
M.pump(60)

# B) 真的发 F2（先让 ListView 拿到焦点并选中第 5 行）
app.win.setTableSelectedRow(pp_app.TBL, 4)
M.pump(60)
user32.SetForegroundWindow(hwnd)
user32.SetFocus(lv)
M.pump(60)
user32.PostMessageW(lv, 0x0100, 0x71, 0)       # WM_KEYDOWN VK_F2
M.pump(150)
print('B 发 F2 之后: selected=%s open=%s row=%s'
      % (app.win.getTableSelectedRow(pp_app.TBL), app._rowEditOpen, app._rowEditRow), flush=True)

# C) 直接看 C++ 有没有回调（用 pending 队列计数）
calls = []
old = app.onGridAction


def probe(action, row, col, text):
    calls.append((action, row, col))
    old(action, row, col, text)


app.win.onGridAction(probe)
M.pump(40)
user32.PostMessageW(lv, 0x0100, 0x71, 0)
M.pump(150)
print('C 捕获到的回调:', calls[-3:], ' open=%s' % app._rowEditOpen, flush=True)
os._exit(0)
