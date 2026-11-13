# -*- coding: utf-8 -*-
"""find_win.py — 枚举所有顶层窗口，打印标题/类名/可见性/矩形；关键字过滤。"""
import ctypes
import sys
from ctypes import wintypes

u = ctypes.windll.user32
kw = sys.argv[1] if len(sys.argv) > 1 else ""

EnumProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


def cls(h):
    b = ctypes.create_unicode_buffer(256)
    u.GetClassNameW(h, b, 256)
    return b.value


def title(h):
    b = ctypes.create_unicode_buffer(512)
    u.GetWindowTextW(h, b, 512)
    return b.value


def rect(h):
    r = wintypes.RECT()
    u.GetWindowRect(h, ctypes.byref(r))
    return (r.left, r.top, r.right, r.bottom)


def walk(h, depth):
    t = title(h)
    if kw and (kw.lower() not in t.lower()):
        return
    print("%s%s h=0x%X vis=%d cls=%s rect=%s"
          % ("  " * depth, "CHILD" if depth else "TOP", h, u.IsWindowVisible(h), cls(h), rect(h)))
    if depth < 2:
        u.EnumChildWindows(h, cb, 0)


@EnumProc
def cb(h, lp):
    t = title(h)
    vis = u.IsWindowVisible(h)
    if t or (kw and kw.lower() in t.lower()):
        print("%sSUB h=0x%X vis=%d cls=%s rect=%s title=%s"
              % ("  " * 1, h, vis, cls(h), rect(h), t))
    elif kw and kw.lower() in cls(h).lower():
        print("SUB h=0x%X vis=%d cls=%s rect=%s" % (h, vis, cls(h), rect(h)))
    return True


@EnumProc
def top(h, lp):
    walk(h, 0)
    return True


u.EnumWindows(top, 0)
