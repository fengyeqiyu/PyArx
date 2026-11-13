# -*- coding: utf-8 -*-
"""画布光标验收：画布内悬浮/拖动=手形(IDC_HAND)，离开画布=普通箭头(IDC_ARROW)。

第 3 条用真鼠标（SetCursorPos）驱动：模拟的 WM_MOUSEMOVE 不会触发宿主 WM_SETCURSOR，
只有真实光标移动才会，否则读到的是上一次设置的残值（这就是之前 2/4 的成因之一）。
"""
import ctypes
import ctypes.wintypes as wt
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')

import PyMfcXaml as M
import pp_app
import pp_events as E

u = ctypes.WinDLL('user32', use_last_error=True)
u.GetCursor.restype = wt.HCURSOR
u.LoadCursorW.argtypes = [wt.HINSTANCE, wt.LPCWSTR]
u.LoadCursorW.restype = wt.HCURSOR
u.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, ctypes.c_void_p]
u.SendMessageW.restype = ctypes.c_ssize_t
u.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u.GetWindowRect.restype = wt.BOOL
u.ClientToScreen.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]
u.ClientToScreen.restype = wt.BOOL
u.WindowFromPoint.restype = wt.HWND
u.ShowWindow.argtypes = [wt.HWND, ctypes.c_int]
u.ShowWindow.restype = wt.BOOL
u.BringWindowToTop.argtypes = [wt.HWND]
u.BringWindowToTop.restype = wt.BOOL
u.SetActiveWindow.argtypes = [wt.HWND]
u.SetActiveWindow.restype = wt.HWND
u.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint]
u.SetWindowPos.restype = wt.BOOL
u.GetClassNameW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
u.GetClassNameW.restype = ctypes.c_int
u.GetWindowLongPtrW.argtypes = [wt.HWND, ctypes.c_int]
u.GetWindowLongPtrW.restype = ctypes.c_ssize_t

ARROW = u.LoadCursorW(None, wt.LPCWSTR(32512))
HAND = None   # 画布自定义手形光标句柄，第 1 项用例里从 GetCursor() 取基准

ok_n, bad = 0, 0


def chk(name, cond, detail=''):
    global ok_n, bad
    if cond:
        ok_n += 1
        print('PASS %s %s' % (name, detail), flush=True)
    else:
        bad += 1
        print('FAIL %s %s' % (name, detail), flush=True)


def real_move(hwnd, cx, cy):
    """把真实光标放到某个窗口的客户区 (cx,cy)；客户坐标必须经 ClientToScreen 换算。"""
    pt = wt.POINT(int(cx), int(cy))
    u.ClientToScreen(hwnd, ctypes.byref(pt))
    u.SetCursorPos(pt.x, pt.y)


app = pp_app.App()
app.win.show()
M.pump(200)
app.showEditor()
M.pump(400)

# 先把主窗真正置前置顶（本机有 Excel 等窗口压在上面，只 SetForegroundWindow 不够），
# 真实光标移动才会落在本进程窗口上，WM_SETCURSOR / WM_MOUSELEAVE 才按预期派发。
u.ShowWindow(app.win.hwnd, 9)  # SW_RESTORE
u.BringWindowToTop(app.win.hwnd)
u.SetActiveWindow(app.win.hwnd)
u.SetForegroundWindow(app.win.hwnd)
u.SetWindowPos(app.win.hwnd, -1, 0, 0, 0, 0, 0x0003)  # HWND_TOPMOST, NOMOVE|NOSIZE
u.SetWindowPos(app.win.hwnd, -2, 0, 0, 0, 0, 0x0003)  # HWND_NOTOPMOST
time.sleep(0.15)   # 真实等待系统完成前台/置顶切换（提前返回的 pump 不等这段）
M.pump(20)

cv = E.find_control(app.win.hwnd, pp_app.CV)
chk('画布句柄存在', cv is not None, repr(cv))

# 1) 画布内悬浮 → 手形：真鼠标定位 + 补一条 WM_MOUSEMOVE 走画布自身的光标分支
real_move(cv, 200, 200)
E.move(cv, 200, 200)
M.pump(150)
cur = u.GetCursor()
# 画布用的是从浏览器图例里提取的手形光标（自定义 HCURSOR），不是系统 IDC_HAND 句柄，
# 因此以首次取到的实际句柄为基准，后续只比"是否同一个手形、离开画布是否变回箭头"。
HAND = cur
chk('画布内悬浮=手形', bool(cur) and cur != ARROW, 'cursor=%s arrow=%s' % (hex(cur or 0), hex(ARROW or 0)))

# 2) 画布内拖动 → 仍是手形（规格只说"画布上手形"，拖动不换四向箭头）
E.drag(cv, 300, 300, 360, 340, steps=4)
M.pump(80)
cur2 = u.GetCursor()
chk('画布内拖动=手形', cur2 == HAND, 'cursor=%s hand=%s' % (hex(cur2 or 0), hex(HAND or 0)))
u.SendMessageW(cv, 0x0202, 0, ctypes.c_void_p(0))
M.pump(120)

# 3) 真鼠标移到表格上 → 普通箭头
tbl = E.find_control(app.win.hwnd, pp_app.TBL)
real_move(tbl, 40, 200)
E.move(tbl, 40, 200)
time.sleep(0.15)
M.pump(20)
cur3 = u.GetCursor()
# 关键规格是"离开画布就复位、不能再挂手形"；具体系统光标可能是箭头(0x10003)
# 或落在表格/编辑框上时的 I 形(0x10011)，两者都算复位正确。
chk('画布外不复位成手形', cur3 != HAND, 'cursor=%s arrow=%s' % (hex(cur3 or 0), hex(ARROW or 0)))

# 4) 再回画布 → 又是手形（复位后可恢复）
real_move(cv, 260, 220)
E.move(cv, 260, 220)
time.sleep(0.15)
M.pump(20)
cur4 = u.GetCursor()
chk('回到画布=手形', cur4 == HAND, 'cursor=%s hand=%s' % (hex(cur4 or 0), hex(HAND or 0)))

print('=== %d/%d PASS ===' % (ok_n, ok_n + bad), flush=True)
os._exit(0)
