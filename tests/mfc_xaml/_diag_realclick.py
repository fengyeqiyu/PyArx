# -*- coding: utf-8 -*-
"""真实鼠标 vs 模拟消息：到底是点位被别的窗口吃掉，还是命中链有问题。"""
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
user32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
user32.WindowFromPoint.argtypes = [wt.POINT]
user32.WindowFromPoint.restype = wt.HWND
user32.ChildWindowFromPointEx.argtypes = [wt.HWND, wt.POINT, wt.UINT]
user32.ChildWindowFromPointEx.restype = wt.HWND
user32.ScreenToClient.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]
user32.ClientToScreen.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]
user32.GetClassNameW.argtypes = [wt.HWND, ctypes.c_wchar_p, ctypes.c_int]
user32.GetWindowLongPtrW.argtypes = [wt.HWND, ctypes.c_int]
user32.GetWindowLongPtrW.restype = ctypes.c_ssize_t
user32.IsWindowVisible.argtypes = [wt.HWND]
user32.SetForegroundWindow.argtypes = [wt.HWND]
user32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                                ctypes.c_int, ctypes.c_int, wt.UINT]
user32.SetCursorPos.argtypes = [ctypes.c_int, ctypes.c_int]
user32.GetCursorPos.argtypes = [ctypes.POINTER(wt.POINT)]
user32.SendInput.argtypes = [ctypes.c_uint, ctypes.c_void_p, ctypes.c_int]
user32.SendInput.restype = ctypes.c_uint


def cls_of(h):
    buf = ctypes.create_unicode_buffer(128)
    user32.GetClassNameW(h, buf, 128)
    cid = user32.GetWindowLongPtrW(h, -12)
    return '%s #%s' % (buf.value, cid)


app = pp_app.App()
app.win.show()
M.pump(120)
app.showEditor()
M.pump(300)

hwnd = app.win.hwnd
user32.SetForegroundWindow(hwnd)
user32.SetWindowPos(hwnd, wt.HWND(-1), 40, 30, 0, 0, 0x0001 | 0x0040)
M.pump(500)

cv = E.find_control(hwnd, pp_app.CV)
wr = wt.RECT()
user32.GetWindowRect(cv, ctypes.byref(wr))
print('画布窗口屏幕矩形 (%d,%d)-(%d,%d)  可见=%s'
      % (wr.left, wr.top, wr.right, wr.bottom, user32.IsWindowVisible(cv)), flush=True)

code = app.nodes[2]['code']
rc = app.win.cardRectPlan(pp_app.CV, code)
print('卡片 %s 客户区 rect=%s' % (code, rc), flush=True)

# 卡片中心 -> 屏幕坐标
pt = wt.POINT(int(rc[0] + rc[2] / 2), int(rc[1] + rc[3] / 2))
user32.ClientToScreen(cv, ctypes.byref(pt))
print('卡片中心屏幕点 = (%d,%d)' % (pt.x, pt.y), flush=True)

h_hit = user32.WindowFromPoint(pt)
print('WindowFromPoint -> %s  (画布=%s)' % (cls_of(h_hit), hex(cv)), flush=True)

# 父链
h = cv
while h:
    h = user32.GetParent(h)
    if h:
        print('  父: %s' % cls_of(h), flush=True)

# 逐点扫：卡片中心上下左右各点，看谁接收
print('--- 采样 ---', flush=True)
for dx, dy in ((0, 0), (-8, 0), (8, 0), (0, -4), (0, 4), (-20, 0), (20, 0)):
    p2 = wt.POINT(pt.x + dx, pt.y + dy)
    hh = user32.WindowFromPoint(p2)
    print('  (%+d,%+d) -> %s' % (dx, dy, cls_of(hh)), flush=True)

# --- SendInput 真实点击 ---
orig = wt.POINT()
user32.GetCursorPos(ctypes.byref(orig))
user32.SetCursorPos(pt.x, pt.y)
time.sleep(0.25)

MOUSEEVENTF_LEFTDOWN = 0x0002
MOUSEEVENTF_LEFTUP = 0x0004


class MOUSEINPUT(ctypes.Structure):
    _fields = [('dx', ctypes.c_long), ('dy', ctypes.c_long),
                ('mouseData', ctypes.c_ulong), ('dwFlags', ctypes.c_ulong),
                ('time', ctypes.c_ulong), ('dwExtraInfo', ctypes.POINTER(ctypes.c_ulong))]


class INPUT(ctypes.Structure):
    _fields = [('type', ctypes.c_ulong), ('mi', MOUSEINPUT)]


def real_click(x, y):
    user32.SetCursorPos(x, y)
    time.sleep(0.12)
    for flag in (MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP):
        inp = INPUT(type=0, mi=MOUSEINPUT(0, 0, 0, flag, 0, None))
        user32.SendInput(1, ctypes.byref(inp), ctypes.sizeof(INPUT))
        time.sleep(0.06)


app.win.selectPlan(pp_app.CV, '')          # 先清空
M.pump(120)
real_click(pt.x, pt.y)
M.pump(250)
sel = app.win.getPlanSelection(pp_app.CV)
print('SendInput 真实点击 (卡片) -> 选中 %r' % sel, flush=True)
print('  详情浮层 visible=%s cur=%s' % (app._detailVisible, app.curSel), flush=True)
user32.SetCursorPos(orig.x, orig.y)

# 再测一次模拟消息点击作对照
app.win.selectPlan(pp_app.CV, '')
M.pump(120)
E.click(cv, int(rc[0] + rc[2] / 2), int(rc[1] + rc[3] / 2))
print('SendMessage 模拟点击 (卡片) -> 选中 %r' % app.win.getPlanSelection(pp_app.CV), flush=True)
os._exit(0)
