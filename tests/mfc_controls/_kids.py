# -*- coding: utf-8 -*-
"""列出某个窗口的全部子窗口：类名/标题/相对父窗口的位置/尺寸/可见性。"""
import sys
import ctypes
from ctypes import wintypes

u = ctypes.windll.user32
u.EnumWindows.argtypes = [ctypes.c_void_p, wintypes.LPARAM]
u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
u.GetWindowTextW.argtypes = [ctypes.c_void_p, wintypes.LPWSTR, ctypes.c_int]
u.GetClassNameW.argtypes = [ctypes.c_void_p, wintypes.LPWSTR, ctypes.c_int]
u.GetWindowThreadProcessId.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.GetDlgCtrlID.argtypes = [ctypes.c_void_p]
u.GetDlgCtrlID.restype = ctypes.c_int
u.IsWindowVisible.argtypes = [ctypes.c_void_p]
u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.MapWindowPoints.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT]


class R(ctypes.Structure):
    _fields_ = [('l', ctypes.c_long), ('t', ctypes.c_long), ('r', ctypes.c_long), ('b', ctypes.c_long)]


TITLE = sys.argv[1]
PID = int(sys.argv[2]) if len(sys.argv) > 2 else 0
EP = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

top = None


def cb(h, _):
    global top
    b = ctypes.create_unicode_buffer(512)
    u.GetWindowTextW(h, b, 512)
    p = ctypes.c_ulong(0)
    u.GetWindowThreadProcessId(h, ctypes.byref(p))
    if TITLE in b.value and (PID == 0 or p.value == PID):
        top = h
        return False
    return True


u.EnumWindows(EP(cb), 0)
if not top:
    print('window not found')
    sys.exit(2)

pr = R()
u.GetWindowRect(top, ctypes.byref(pr))
print('top %s rect=(%d,%d)-(%d,%d) size=%dx%d' % (hex(top), pr.l, pr.t, pr.r, pr.b, pr.r - pr.l, pr.b - pr.t))


def onc(h, _):
    c = ctypes.create_unicode_buffer(256)
    u.GetClassNameW(h, c, 256)
    b = ctypes.create_unicode_buffer(512)
    u.GetWindowTextW(h, b, 512)
    r = R()
    u.GetWindowRect(h, ctypes.byref(r))
    pt = R()
    pt.l, pt.t = r.l, r.t
    u.MapWindowPoints(None, top, ctypes.byref(pt), 2)
    print('  id=%-5d %-22s vis=%d rel=(%4d,%4d) size=%dx%d  text=%s'
          % (u.GetDlgCtrlID(h), c.value, u.IsWindowVisible(h), pt.l, pt.t, r.r - r.l, r.b - r.t, b.value))
    return True


u.EnumChildWindows(top, EP(onc), 0)
