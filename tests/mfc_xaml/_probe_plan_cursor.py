# -*- coding: utf-8 -*-
"""在真实运行的配电图应用上验证：画布上悬浮=grab、按住左键拖动=grabbing、松开回 grab。"""
import ctypes
import sys
import time
from ctypes import wintypes as wt

TITLE = '配电规划图'
u = ctypes.windll.user32
u.GetCursorInfo.argtypes = [ctypes.c_void_p]
u.GetCursorInfo.restype = wt.BOOL
u.LoadCursorW.argtypes = [wt.HINSTANCE, ctypes.c_void_p]
u.LoadCursorW.restype = wt.HANDLE
u.SetCursorPos.argtypes = [ctypes.c_int, ctypes.c_int]
u.mouse_event.argtypes = [wt.DWORD, wt.DWORD, wt.DWORD, wt.DWORD, wt.DWORD]
WNDENUMPROC = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
u.EnumWindows.argtypes = [WNDENUMPROC, wt.LPARAM]


class CURSORINFO(ctypes.Structure):
    _fields_ = [('cbSize', wt.DWORD), ('flags', wt.DWORD), ('hCursor', wt.HANDLE), ('pt', wt.POINT)]


def find():
    hits = []

    def cb(hwnd, _):
        if u.IsWindowVisible(hwnd):
            t = ctypes.create_unicode_buffer(256)
            u.GetWindowTextW(hwnd, t, 256)
            if TITLE in t.value:
                r = wt.RECT()
                u.GetWindowRect(hwnd, ctypes.byref(r))
                hits.append((hwnd, t.value, r))
        return True

    u.EnumWindows(WNDENUMPROC(cb), 0)
    return hits


def cursor():
    ci = CURSORINFO()
    ci.cbSize = ctypes.sizeof(CURSORINFO)
    u.GetCursorInfo(ctypes.byref(ci))
    return ci.hCursor


def main():
    wins = find()
    if not wins:
        print('app not running')
        return 1
    hwnd, title, r = wins[0]
    print('window', title, r.left, r.top, r.right, r.bottom)
    cr = wt.RECT()
    u.GetClientRect.argtypes = [wt.HWND, ctypes.c_void_p]
    u.GetClientRect(hwnd, ctypes.byref(cr))
    org = wt.POINT(0, 0)
    u.ClientToScreen.argtypes = [wt.HWND, ctypes.c_void_p]
    u.ClientToScreen(hwnd, ctypes.byref(org))
    u.SetForegroundWindow(hwnd)
    time.sleep(0.8)
    # 画布大致在客户区左侧 60% 的中部
    x = org.x + int(cr.right * 0.30)
    y = org.y + int(cr.bottom * 0.45)
    u.SetCursorPos(x, y)
    time.sleep(0.6)
    h_hover = cursor()
    u.mouse_event(0x0002, 0, 0, 0, 0)
    time.sleep(0.4)
    h_drag = cursor()
    for _ in range(6):
        u.mouse_event(0x0001, 6, 4, 0, 0)
        time.sleep(0.05)
    time.sleep(0.3)
    h_drag2 = cursor()
    u.mouse_event(0x0004, 0, 0, 0, 0)
    time.sleep(0.4)
    h_up = cursor()
    arrow = u.LoadCursorW(None, ctypes.c_void_p(32512))
    hand = u.LoadCursorW(None, ctypes.c_void_p(32649))
    print('hover   %#x' % (h_hover or 0))
    print('down    %#x' % (h_drag or 0))
    print('dragged %#x' % (h_drag2 or 0))
    print('up      %#x' % (h_up or 0))
    print('sys arrow %#x hand %#x' % (arrow or 0, hand or 0))
    ok1 = h_hover and h_hover not in (arrow, hand)
    ok2 = h_drag and h_drag not in (arrow, hand) and h_drag != h_hover
    ok3 = h_up == h_hover
    print('CHECK hover=grab-custom:', bool(ok1))
    print('CHECK down=grabbing-custom(不同):', bool(ok2))
    print('CHECK up 回 grab:', bool(ok3))
    return 0 if (ok1 and ok2 and ok3) else 1


if __name__ == '__main__':
    sys.exit(main())
