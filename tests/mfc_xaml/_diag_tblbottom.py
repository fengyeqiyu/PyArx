# -*- coding: utf-8 -*-
"""表格底部为什么露出"半行蓝色"：量表格高度、行高、表头高，看是否整行对齐。"""
import ctypes
import ctypes.wintypes as wt
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')

import PyMfcXaml as M
import pp_app
import pp_events as E

user32 = ctypes.WinDLL('user32', use_last_error=True)
user32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, ctypes.c_void_p]
user32.SendMessageW.restype = ctypes.c_ssize_t
user32.GetClientRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
user32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
user32.ClientToScreen.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]

LVM_FIRST = 0x1000
LVM_GETITEMRECT = LVM_FIRST + 14
LVM_GETCOUNTPERPAGE = LVM_FIRST + 40
LVM_GETHEADER = LVM_FIRST + 31
LVM_GETTOPINDEX = LVM_FIRST + 27

app = pp_app.App()
app.win.show()
M.pump(200)
app.showEditor()
M.pump(300)
# 打开一个行编辑条，复现用户截图的状态
app.win.setTableSelectedRow(pp_app.TBL, 33)
app._openRowEdit(33)
M.pump(300)

tbl = E.find_control(app.win.hwnd, pp_app.TBL)
rc = wt.RECT()
user32.GetClientRect(tbl, ctypes.byref(rc))
print('表格客户区高 = %d' % rc.bottom, flush=True)

item = (ctypes.c_int * 4)(0, 1, 0, 0)          # left, top, right, bottom
res = user32.SendMessageW(tbl, LVM_GETITEMRECT, 0, ctypes.byref(item))
print('第 0 行 item rect = (%d,%d,%d,%d)  行高 = %d' % (item[0], item[1], item[2], item[3],
                                                      item[3] - item[1]), flush=True)

hdr = user32.SendMessageW(tbl, LVM_GETHEADER, 0, None)
hr = wt.RECT()
if hdr:
    user32.GetWindowRect(wt.HWND(hdr), ctypes.byref(hr))
    print('表头高 = %d' % (hr.bottom - hr.top), flush=True)

n = user32.SendMessageW(tbl, LVM_GETCOUNTPERPAGE, 0, None)
ti = user32.SendMessageW(tbl, LVM_GETTOPINDEX, 0, None)
print('每页可显示行数 = %d  当前首行索引 = %d  总行 = %d' % (n, ti, app.win.getTableRowCount(pp_app.TBL)),
      flush=True)

rowh = item[3] - item[1]
hdr_w = hr.bottom - hr.top if hdr else 0
body_h = rc.bottom - hdr_w
print('正文字区高 = %d  -> %.2f 行' % (body_h, body_h / float(rowh)), flush=True)
print('余量 = %d px  （非 0 就会露出半行）' % (body_h % rowh), flush=True)

pt = wt.POINT(0, 0)
user32.ClientToScreen(tbl, ctypes.byref(pt))
print('表格左上屏幕坐标 = (%d,%d)  尺寸 %dx%d' % (pt.x, pt.y, rc.right, rc.bottom), flush=True)
os._exit(0)
