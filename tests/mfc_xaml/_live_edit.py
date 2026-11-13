# -*- coding: utf-8 -*-
import os, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import PyMfcXaml as M
import pp_app

app = pp_app.App(); app.win.show(); M.pump(80)
app.showEditor(); M.pump(200)
# 选中第 3 行并进入第 2 列原位编辑，供屏幕截图核对对齐
app.win.tableSetSelectedRow(pp_app.TBL, 2) if app._newTable else app.win.setTableSelectedRow(pp_app.TBL, 2)
M.pump(40)
app._cellBegin(2, 1)
M.pump(120)

import ctypes, ctypes.wintypes as wt
u = ctypes.WinDLL('user32', use_last_error=True)
u.SetForegroundWindow.argtypes = [wt.HWND]
u.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, wt.UINT]
u.SetForegroundWindow(app.win.hwnd)
u.SetWindowPos(app.win.hwnd, wt.HWND(-1), 100, 100, 0, 0, 0x0001 | 0x0040)
time.sleep(1.0)

while True:
    M.pump(50)
