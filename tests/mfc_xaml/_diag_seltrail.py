# -*- coding: utf-8 -*-
"""表格选中行的拖影/刷新量诊断：连续切行并计时、抓图查是否残留多行蓝色。"""
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
import pp_events as E

user32 = ctypes.WinDLL('user32', use_last_error=True)
user32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                                ctypes.c_int, ctypes.c_int, wt.UINT]

app = pp_app.App()
app.win.show()
M.pump(100)
app.showEditor()
M.pump(250)

tbl = E.find_control(app.win.hwnd, pp_app.TBL)
user32.SetWindowPos(app.win.hwnd, wt.HWND(-1), 40, 30, 0, 0, 0x0001 | 0x0040)
M.pump(400)

rows = app._grid_rows
print('表格行数 %d' % rows, flush=True)

# 1. 连续切换选中行，量耗时（旧实现是 39 行全量 SetItemState）
t0 = time.perf_counter()
for r in (0, 12, 25, 5, 33, 18, 2, 30):
    app.win.setTableSelectedRow(pp_app.TBL, r)
    M.pump(1)
t1 = time.perf_counter()
print('8 次切行耗时 %.1f ms（平均 %.1f ms/次）'
      % ((t1 - t0) * 1000, (t1 - t0) * 1000 / 8), flush=True)

# 2. 选中两行不同位置，各抓一张屏幕图，看是否出现"多行蓝"
for r, name in ((3, '_trail_a.png'), (25, '_trail_b.png')):
    app.win.setTableSelectedRow(pp_app.TBL, r)
    M.pump(60)
    user32.SetWindowPos(app.win.hwnd, wt.HWND(-1), 40, 30, 0, 0, 0x0001 | 0x0040)
    time.sleep(0.35)
    app.win.savePng(r'G:\PyArx\docs\%s' % name)
    print('截图 %s -> row %d (%s B)'
          % (name, r, os.path.getsize(r'G:\PyArx\docs\%s' % name)), flush=True)
    # 抓完立刻读选中行，确认模型侧只有一行
    print('  getTableSelectedRow=%s' % app.win.getTableSelectedRow(pp_app.TBL), flush=True)

os._exit(0)
