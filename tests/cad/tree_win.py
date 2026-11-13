# -*- coding: utf-8 -*-
"""tree_win.py — 打印指定父窗口的子树（类名/标题/可见/矩形）。用法: tree_win.py <hwnd十进制或0x>"""
import ctypes
import sys
from ctypes import wintypes

u = ctypes.windll.user32
h0 = int(sys.argv[1], 0)

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
    return "%d,%d,%d,%d" % (r.left, r.top, r.right, r.bottom)


def dump(h, depth, maxd):
    u.EnumChildWindows(h, cb, depth, maxd)


@EnumProc
def cb(h, lp):
    return True


def walk(h, depth):
    print("%s0x%X vis=%d cls=%s rect=%s title=%r"
          % ("  " * depth, h, u.IsWindowVisible(h), cls(h), rect(h), title(h)))
    if depth >= 4:
        return
    kids = []

    @EnumProc
    def each(k, lp):
        kids.append(k)
        return True

    u.EnumChildWindows(h, each, 0)
    for k in kids:
        walk(k, depth + 1)


walk(h0, 0)
