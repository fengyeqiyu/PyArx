# -*- coding: utf-8 -*-
"""谁盖住了画布：枚举该点下的兄弟窗口（Z 序 / 样式 / 区域）。"""
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
user32.GetWindow.argtypes = [wt.HWND, wt.UINT]
user32.GetWindow.restype = wt.HWND
user32.GetClassNameW.argtypes = [wt.HWND, ctypes.c_wchar_p, ctypes.c_int]
user32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
user32.IsWindowVisible.argtypes = [wt.HWND]
user32.GetWindowLongPtrW.argtypes = [wt.HWND, ctypes.c_int]
user32.GetWindowLongPtrW.restype = ctypes.c_ssize_t
user32.ScreenToClient.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]
user32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                                ctypes.c_int, ctypes.c_int, wt.UINT]

GW_CHILD, GW_HWNDNEXT, GW_HWNDPREV, GWL_STYLE, GWL_EXSTYLE, GWL_ID = 5, 2, 3, -16, -20, -12


def info(h):
    buf = ctypes.create_unicode_buffer(128)
    user32.GetClassNameW(h, buf, 128)
    wr = wt.RECT()
    user32.GetWindowRect(h, ctypes.byref(wr))
    st = user32.GetWindowLongPtrW(h, GWL_STYLE)
    ex = user32.GetWindowLongPtrW(h, GWL_EXSTYLE)
    cid = user32.GetWindowLongPtrW(h, GWL_ID)
    return ('id=%-5s %-22s (%4d,%4d)-(%4d,%4d) vis=%d style=0x%08X ex=0x%08X'
            % (cid, buf.value, wr.left, wr.top, wr.right, wr.bottom,
               user32.IsWindowVisible(h), st & 0xFFFFFFFF, ex & 0xFFFFFFFF))


app = pp_app.App()
app.win.show()
M.pump(120)
app.showEditor()
M.pump(300)
hwnd = app.win.hwnd
user32.SetWindowPos(hwnd, wt.HWND(-1), 40, 30, 0, 0, 0x0001 | 0x0040)
M.pump(500)

cv = E.find_control(hwnd, pp_app.CV)
print('画布 %s -> %s' % (hex(cv), info(cv)), flush=True)

rc = app.win.cardRectPlan(pp_app.CV, app.nodes[2]['code'])
pt = wt.POINT(int(rc[0] + rc[2] / 2), int(rc[1] + rc[3] / 2))
user32.ClientToScreen(cv, ctypes.byref(pt))
print('测试点 (%d,%d)' % (pt.x, pt.y), flush=True)

# 逐层遍历：主窗 -> 容器 -> ...
h_level = hwnd
depth = 0
while h_level and depth < 4:
    print('\n=== 层级 %d：%s 的子窗口（Z 序，从上到下）===' % (depth, info(h_level)), flush=True)
    h = user32.GetWindow(h_level, GW_CHILD)
    # GetWindow(GW_CHILD) 给的是 Z 序最底（先建）… 用反向遍历更直观
    kids = []
    while h:
        kids.append(h)
        h = user32.GetWindow(h, GW_HWNDNEXT)
    in_point = []
    for k in kids:
        wr = wt.RECT()
        user32.GetWindowRect(k, ctypes.byref(wr))
        if wr.left <= pt.x < wr.right and wr.top <= pt.y < wr.bottom:
            in_point.append(k)
    for k in kids:
        mark = '  <== 覆盖测试点' if k in in_point else ''
        print('  ' + info(k) + mark, flush=True)
    print('  覆盖测试点的子控件：%d 个' % len(in_point), flush=True)
    # 下一层：优先跟随覆盖点、且是 in_point 里最后一个（Z 序最上）
    nxt = in_point[-1] if in_point else None
    if nxt is None:
        break
    h_level = nxt
    depth += 1

os._exit(0)
