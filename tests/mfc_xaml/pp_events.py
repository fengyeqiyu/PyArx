# -*- coding: utf-8 -*-
"""模拟鼠标/键盘事件驱动控件 —— 不需要真人操作，脚本即可验证交互。

用法：
    import pp_events as E
    cv = E.find_control(app.win.hwnd, ppApp.CV)
    E.click(cv, 140, 389)          # 点画布里的卡片
    E.wheel(cv, 120, 400, 300)     # 在 (400,300) 处滚一格
    E.drag(cv, 400, 300, 600, 420) # 左键按住拖动
    E.key(tbl, 0x71)               # F2 打开行编辑条（VK_F2 = 0x71，不是 0x72）

要点：
* 全部走 user32.SendMessageW 直接投递到控件过程，不移动真鼠标、不抢焦点；
* lParam 第 4 参数必须 cVoidP/整型，用 wt.LPARAM 会让 byref(rc) 报错；
* 找控件句柄要递归遍历（GetDlgItem 取不到自绘画布）；
* x64 只能用 GetWindowLongPtrW（没有 GetWindowLong）。
"""
import ctypes
import ctypes.wintypes as wt

import PyMfcXaml as M

user32 = ctypes.WinDLL('user32', use_last_error=True)
user32.GetWindow.argtypes = [wt.HWND, wt.UINT]
user32.GetWindow.restype = wt.HWND
user32.GetWindowLongPtrW.argtypes = [wt.HWND, ctypes.c_int]
user32.GetWindowLongPtrW.restype = ctypes.c_ssize_t
user32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.SendMessageW.restype = ctypes.c_ssize_t
user32.ClientToScreen.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]
user32.ClientToScreen.restype = wt.BOOL

GW_CHILD, GW_HWNDNEXT, GWL_ID = 5, 2, -12

WM_MOUSEMOVE = 0x0200
WM_LBUTTONDOWN = 0x0201
WM_LBUTTONUP = 0x0202
WM_LBUTTONDBLCLK = 0x0203
WM_MOUSEWHEEL = 0x020A
WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
WM_CHAR = 0x0102

MK_LBUTTON = 0x0001


def find_control(parent, ctrl_id):
    """按控件 id 递归找 HWND（画布/自绘控件用 GetDlgItem 取不到）。"""
    h = user32.GetWindow(parent, GW_CHILD)
    while h:
        if user32.GetWindowLongPtrW(h, GWL_ID) == ctrl_id:
            return h
        sub = find_control(h, ctrl_id)
        if sub:
            return sub
        h = user32.GetWindow(h, GW_HWNDNEXT)
    return 0


def _lp(x, y):
    return (int(y) << 16) | (int(x) & 0xFFFF)


def move(hwnd, x, y, buttons=0):
    user32.SendMessageW(hwnd, WM_MOUSEMOVE, buttons, _lp(x, y))
    M.pump(10)


def click(hwnd, x, y, pump=80):
    """一次单击：down + up 落在同一像素上（不会触发拖动判定）。"""
    user32.SendMessageW(hwnd, WM_MOUSEMOVE, 0, _lp(x, y))
    user32.SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, _lp(x, y))
    M.pump(20)
    user32.SendMessageW(hwnd, WM_LBUTTONUP, 0, _lp(x, y))
    M.pump(pump)


def double_click(hwnd, x, y, pump=80):
    user32.SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, _lp(x, y))
    user32.SendMessageW(hwnd, WM_LBUTTONUP, 0, _lp(x, y))
    user32.SendMessageW(hwnd, WM_LBUTTONDBLCLK, MK_LBUTTON, _lp(x, y))
    user32.SendMessageW(hwnd, WM_LBUTTONUP, 0, _lp(x, y))
    M.pump(pump)


def _screen_pt(hwnd, x, y):
    """客户坐标 → 屏幕坐标。"""
    pt = wt.POINT(int(x), int(y))
    user32.ClientToScreen(hwnd, ctypes.byref(pt))
    return pt.x, pt.y


def _client_of_screen(hwnd, sx, sy):
    """屏幕坐标 → **当下这一刻**的客户坐标。

    拖动中控件自己会移位，客户坐标的零点跟着走；所以每一步都要按当下的原点换算，
    而不是沿用开始时的换算结果。
    """
    pt = wt.POINT(int(sx), int(sy))
    user32.ScreenToClient(hwnd, ctypes.byref(pt))
    return pt.x, pt.y


def _move_cursor(sx, sy):
    user32.SetCursorPos(int(sx), int(sy))


def drag(hwnd, x1, y1, x2, y2, steps=6, pump=60):
    """左键按住从 (x1,y1) 拖到 (x2,y2)；中间插 mousemove 让控件看到移动。

    起点终点先换算成**屏幕坐标**再走：控件 SetCapture 之后，系统会把真实鼠标的
    移动也发给它，真实光标不跟着走的话会有坐标完全不同的事件挤进来；而控件在
    拖动中移位又会让客户坐标漂移。所以整条轨迹定在屏幕坐标上，每一步按控件当下
    的原点折回客户坐标发消息，并顺手把真实光标挪到同一个屏幕点上。
    """
    sx1, sy1 = _screen_pt(hwnd, x1, y1)
    sx2, sy2 = _screen_pt(hwnd, x2, y2)
    cx, cy = _client_of_screen(hwnd, sx1, sy1)
    _move_cursor(sx1, sy1)
    user32.SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, _lp(cx, cy))
    M.pump(20)
    for i in range(1, steps + 1):
        sx = int(sx1 + (sx2 - sx1) * i / steps)
        sy = int(sy1 + (sy2 - sy1) * i / steps)
        cx, cy = _client_of_screen(hwnd, sx, sy)
        _move_cursor(sx, sy)
        user32.SendMessageW(hwnd, WM_MOUSEMOVE, MK_LBUTTON, _lp(cx, cy))
        M.pump(20)
    cx, cy = _client_of_screen(hwnd, sx2, sy2)
    _move_cursor(sx2, sy2)
    user32.SendMessageW(hwnd, WM_LBUTTONUP, 0, _lp(cx, cy))
    M.pump(pump)


def wheel(hwnd, delta, x, y, pump=80):
    """滚轮：delta>0 放大、<0 缩小。lParam 用屏幕坐标，高字是滚动量。"""
    pt = wt.POINT(int(x), int(y))
    user32.ClientToScreen(hwnd, ctypes.byref(pt))
    key = (int(delta) & 0xFFFF) << 16
    user32.SendMessageW(hwnd, WM_MOUSEWHEEL, key, _lp(pt.x, pt.y))
    M.pump(pump)


def key(hwnd, vk, pump=60):
    user32.SendMessageW(hwnd, WM_KEYDOWN, vk, 0x00000001)
    user32.SendMessageW(hwnd, WM_KEYUP, vk, 0xC0000001)
    M.pump(pump)


def key_with_ctrl(hwnd, vk, pump=60):
    user32.SendMessageW(hwnd, WM_KEYDOWN, vk, 0x001D0001)   # 伪 Ctrl 状态位
    user32.SendMessageW(hwnd, WM_KEYUP, vk, 0xC01D0001)
    M.pump(pump)


def type_text(hwnd, text, pump=20):
    for ch in text:
        user32.SendMessageW(hwnd, WM_CHAR, ord(ch), 0x00000001)
        M.pump(pump)
