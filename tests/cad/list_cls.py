# -*- coding: utf-8 -*-
"""list_cls.py — 列出全部顶层窗口的 类名/可见/矩形/标题，可按类名关键字过滤。"""
import ctypes
import sys
from ctypes import wintypes

u = ctypes.windll.user32
kw = (sys.argv[1] if len(sys.argv) > 1 else "").lower()
EnumProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


def cls(h):
    b = ctypes.create_unicode_buffer(256)
    u.GetClassNameW(h, b, 256)
    return b.value


def title(h):
    b = ctypes.create_unicode_buffer(512)
    u.GetWindowTextW(h, b, 512)
    return b.value


@EnumProc
def cb(h, lp):
    c = cls(h)
    if kw and kw not in c.lower() and kw not in title(h).lower():
        return True
    r = wintypes.RECT()
    u.GetWindowRect(h, ctypes.byref(r))
    print("0x%X vis=%d cls=%s rect=%d,%d,%d,%d title=%r"
          % (h, u.IsWindowVisible(h), c, r.left, r.top, r.right, r.bottom, title(h)))
    return True


u.EnumWindows(cb, 0)
