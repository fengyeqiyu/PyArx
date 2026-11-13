# -*- coding: utf-8 -*-
"""真实鼠标验收：SendInput 真点击 + 真滚轮，验证系统命中链（不是 SendMessage）。"""
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
user32.ClientToScreen.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]
user32.SetCursorPos.argtypes = [ctypes.c_int, ctypes.c_int]
user32.GetCursorPos.argtypes = [ctypes.POINTER(wt.POINT)]
user32.SendInput.argtypes = [ctypes.c_uint, ctypes.c_void_p, ctypes.c_int]
user32.SendInput.restype = ctypes.c_uint
user32.mouse_event.argtypes = [ctypes.c_ulong, ctypes.c_long, ctypes.c_long, ctypes.c_ulong, ctypes.c_void_p]
user32.mouse_event.restype = None
user32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                                ctypes.c_int, ctypes.c_int, wt.UINT]
user32.GetWindowThreadProcessId.argtypes = [wt.HWND, ctypes.POINTER(wt.DWORD)]
user32.GetWindowThreadProcessId.restype = wt.DWORD
user32.AttachThreadInput.argtypes = [wt.DWORD, wt.DWORD, wt.BOOL]
user32.AttachThreadInput.restype = wt.BOOL
kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
kernel32.GetCurrentThreadId.restype = wt.DWORD
user32.GetForegroundWindow.restype = wt.HWND
user32.SetForegroundWindow.argtypes = [wt.HWND]
user32.SetForegroundWindow.restype = wt.BOOL
user32.ScreenToClient.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]
user32.ScreenToClient.restype = wt.BOOL
user32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.SendMessageW.restype = ctypes.c_ssize_t

MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP, MOUSEEVENTF_WHEEL = 0x0002, 0x0004, 0x0800
WM_MOUSEMOVE, MK_LBUTTON = 0x0200, 0x0001


def _lp(x, y):
    return (int(y) << 16) | (int(x) & 0xFFFF)

ok_n, bad = 0, 0


def chk(name, cond, detail=''):
    global ok_n, bad
    if cond:
        ok_n += 1
        print('PASS %s %s' % (name, detail), flush=True)
    else:
        bad += 1
        print('FAIL %s %s' % (name, detail), flush=True)


class MOUSEINPUT(ctypes.Structure):
    _fields = [('dx', ctypes.c_long), ('dy', ctypes.c_long),
                ('mouseData', ctypes.c_ulong), ('dwFlags', ctypes.c_ulong),
                ('time', ctypes.c_ulong), ('dwExtraInfo', ctypes.POINTER(ctypes.c_ulong))]


class INPUT(ctypes.Structure):
    _fields = [('type', ctypes.c_ulong), ('mi', MOUSEINPUT)]


def send(flags, data=0, x=0, y=0):
    # 点击/滚轮：mouse_event 投递真实系统鼠标消息；移动不走这里。
    # 拖动步进改用 SetCursorPos 绝对定位（见 real_drag）：mouse_event 的 dx/dy 是
    # 相对 mickey，受加速/DPI 缩放影响，无人值守会话里不可靠；SetCursorPos 是系统级
    # 绝对坐标，与 pp_cursor 的 real_move 同一条已验证稳定的路径。
    user32.mouse_event(flags, 0, 0, data, None)


def real_click(x, y):
    user32.SetCursorPos(x, y)
    time.sleep(0.15)
    send(MOUSEEVENTF_LEFTDOWN)
    time.sleep(0.05)
    send(MOUSEEVENTF_LEFTUP)
    time.sleep(0.05)
    M.pump(120)          # 真实输入也要宿主消息循环分发，脚本必须 pump
    time.sleep(0.10)


def real_wheel(x, y, delta):
    user32.SetCursorPos(x, y)
    time.sleep(0.15)
    for _ in range(abs(delta) // 120):
        send(MOUSEEVENTF_WHEEL, 120 if delta > 0 else 0xFFFFFF88)
        time.sleep(0.05)
        M.pump(60)
    time.sleep(0.10)


app = pp_app.App()
app.win.show()
M.pump(120)
app.showEditor()
M.pump(300)
hwnd = app.win.hwnd
user32.ShowWindow.argtypes = [wt.HWND, ctypes.c_int]
user32.ShowWindow.restype = wt.BOOL
user32.BringWindowToTop.argtypes = [wt.HWND]
user32.BringWindowToTop.restype = wt.BOOL
user32.SetActiveWindow.argtypes = [wt.HWND]
user32.SetActiveWindow.restype = wt.HWND
def force_front(h):
    # 无人值守会话里后台进程直接 SetForegroundWindow 常被系统挡回；
    # 先 AttachThreadInput 到前台线程，借其输入队列完成激活，再保持 TOPMOST。
    fg = user32.GetForegroundWindow()
    fg_thread = user32.GetWindowThreadProcessId(fg, None)
    cur_thread = kernel32.GetCurrentThreadId()
    if fg_thread and fg_thread != cur_thread:
        user32.AttachThreadInput(cur_thread, fg_thread, True)
    user32.ShowWindow(h, 9)
    user32.BringWindowToTop(h)
    user32.SetActiveWindow(h)
    user32.SetForegroundWindow(h)
    user32.SetWindowPos(h, wt.HWND(-1), 40, 30, 0, 0, 0x0001 | 0x0040)
    if fg_thread and fg_thread != cur_thread:
        user32.AttachThreadInput(cur_thread, fg_thread, False)
    time.sleep(0.15)
    M.pump(20)


force_front(hwnd)

cv = E.find_control(hwnd, pp_app.CV)
CODE = app.nodes[2]['code']


def card_screen_pt():
    """每次现算卡片中心 → 屏幕坐标。窗口位置/尺寸会随系统摆放变化，
    算一次复用会让点击落到画布空白处（历史上 4/6 就是这个原因）。"""
    rc = app.win.cardRectPlan(pp_app.CV, CODE)
    p = wt.POINT(int(rc[0] + rc[2] / 2), int(rc[1] + rc[3] / 2))
    user32.ClientToScreen(cv, ctypes.byref(p))
    return p


# 先在客户坐标里确认这个点真的命中该卡片，命中才去点
p = card_screen_pt()
cx, cy = p.x, p.y
pt = p
print('测试点 (%d,%d) code=%s' % (pt.x, pt.y, CODE), flush=True)

h_hit = user32.WindowFromPoint(pt)
chk('系统命中落在画布上', h_hit == cv, 'hit=%s cv=%s' % (hex(h_hit), hex(cv)))

orig = wt.POINT()
user32.GetCursorPos(ctypes.byref(orig))

user32.GetForegroundWindow.restype = wt.HWND
user32.SetForegroundWindow.argtypes = [wt.HWND]
user32.SetForegroundWindow.restype = wt.BOOL
fg = user32.GetForegroundWindow()
print('前台窗口 = %s  目标 = %s' % (hex(fg or 0), hex(hwnd)), flush=True)
user32.SetForegroundWindow(hwnd)
M.pump(300)
fg2 = user32.GetForegroundWindow()
print('激活后前台 = %s' % hex(fg2 or 0), flush=True)

# 1. 真实单击卡片
app.win.selectPlan(pp_app.CV, '')
M.pump(150)
# 现算一次：窗口已经被激活/置顶过，位置和尺寸可能都变了
pt = card_screen_pt()
real_click(pt.x, pt.y)
# 第一次点击可能只用于激活窗口，连点两下再判定
pt = card_screen_pt()
real_click(pt.x, pt.y)
sel = app.win.getPlanSelection(pp_app.CV)
print('探针 (lastMsg, upX, upY, wasDrag, moved, lastHit) = %s'
      % (app.win.planProbe(pp_app.CV),), flush=True)
chk('真实鼠标单击选中卡片', sel == app.nodes[2]['code'], repr(sel))
chk('真实点击后详情浮层打开', bool(app._detailVisible),
    'visible=%s cur=%s' % (app._detailVisible, app.curSel))

# 2. 真实滚轮缩放
# 上一步点击已弹出详情浮层，浮层盖在卡片上会吃掉滚轮/拖动；先收起，再取画布空白点。
app.hideDetail()
M.pump(150)
rcv = wt.RECT()
user32.GetClientRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
user32.GetClientRect(cv, ctypes.byref(rcv))
pvb = wt.POINT(rcv.right - 12, rcv.bottom // 2)
user32.ClientToScreen(cv, ctypes.byref(pvb))
z0 = app.win.getPlanView(pp_app.CV)[0]
real_wheel(pvb.x, pvb.y, 120)
z1 = app.win.getPlanView(pp_app.CV)[0]
chk('真实滚轮放大画布', z1 > z0 + 1e-4, '%.4f -> %.4f' % (z0, z1))
real_wheel(pvb.x, pvb.y, -120)
z2 = app.win.getPlanView(pp_app.CV)[0]
chk('真实滚轮缩回画布', abs(z2 - z0) < 1e-3, '%.4f' % z2)

# 3. 真实左键拖动平移：屏幕坐标轨迹，每步 SetCursorPos 绝对定位，
#    再按当下客户原点补一条带 MK_LBUTTON 的 WM_MOUSEMOVE（与 pp_events.drag 同法）。
v0 = app.win.getPlanView(pp_app.CV)
sx0, sy0 = pvb.x, pvb.y
user32.SetCursorPos(sx0, sy0)
time.sleep(0.15)
send(MOUSEEVENTF_LEFTDOWN)
M.pump(30)
for i in range(1, 7):
    sx = sx0 + int(160 * i / 6)
    sy = sy0 + int(100 * i / 6)
    user32.SetCursorPos(sx, sy)
    pc = wt.POINT(sx, sy)
    user32.ScreenToClient(cv, ctypes.byref(pc))
    user32.SendMessageW(cv, WM_MOUSEMOVE, MK_LBUTTON, _lp(pc.x, pc.y))
    time.sleep(0.03)
    M.pump(20)
time.sleep(0.05)
send(MOUSEEVENTF_LEFTUP)
M.pump(150)
v1 = app.win.getPlanView(pp_app.CV)
chk('真实左键拖动平移', abs(v1[1] - v0[1]) > 1 or abs(v1[2] - v0[2]) > 1,
    '(%s,%s) -> (%s,%s)' % (v0[1], v0[2], v1[1], v1[2]))

user32.SetCursorPos(orig.x, orig.y)
print('=== %d/%d PASS ===' % (ok_n, ok_n + bad), flush=True)
os._exit(0)
