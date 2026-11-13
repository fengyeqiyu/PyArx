# -*- coding: utf-8 -*-
"""按窗口标题抓一个"每控件一子窗口"的自绘窗（父窗口 PrintWindow + 子窗口 WM_PRINTCLIENT）。

用法：python _grab_title.py <标题子串> <输出bmp>
"""
import ctypes
import os
import sys
from ctypes import wintypes

TITLE = sys.argv[1]
OUT = sys.argv[2]

u = ctypes.windll.user32
g = ctypes.windll.gdi32

u.EnumWindows.argtypes = [ctypes.c_void_p, wintypes.LPARAM]
u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
u.GetWindowTextW.argtypes = [ctypes.c_void_p, wintypes.LPWSTR, ctypes.c_int]
u.GetDlgCtrlID.argtypes = [ctypes.c_void_p]
u.GetDlgCtrlID.restype = ctypes.c_int
u.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
u.SendMessageW.restype = wintypes.LPARAM
u.PrintWindow.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT]
u.PrintWindow.restype = wintypes.BOOL
u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.MapWindowPoints.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT]
g.SetViewportOrgEx.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_void_p]
g.CreateCompatibleDC.argtypes = [ctypes.c_void_p]
g.CreateCompatibleDC.restype = ctypes.c_void_p
g.CreateDIBSection.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD,
                               ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD]
g.CreateDIBSection.restype = ctypes.c_void_p
g.SelectObject.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
g.SelectObject.restype = ctypes.c_void_p

WM_PRINTCLIENT = 0x0318
PRF_CLIENT = 0x00000004


class R(ctypes.Structure):
    _fields_ = [('l', ctypes.c_long), ('t', ctypes.c_long), ('r', ctypes.c_long), ('b', ctypes.c_long)]


class BMIH(ctypes.Structure):
    _fields_ = [('biSize', wintypes.DWORD), ('biWidth', ctypes.c_long), ('biHeight', ctypes.c_long),
                ('biPlanes', wintypes.WORD), ('biBitCount', wintypes.WORD),
                ('biCompression', wintypes.DWORD), ('biSizeImage', wintypes.DWORD),
                ('biXPelsPerMeter', ctypes.c_long), ('biYPelsPerMeter', ctypes.c_long),
                ('biClrUsed', wintypes.DWORD), ('biClrImportant', wintypes.DWORD)]

found = []
EP = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


def on_top(h, _):
    buf = ctypes.create_unicode_buffer(512)
    u.GetWindowTextW(h, buf, 512)
    if TITLE in buf.value:
        found.append((h, buf.value))
    return True


u.EnumWindows(EP(on_top), 0)
if not found:
    print('window not found: %s' % TITLE)
    sys.exit(2)
hwnd, title = found[0]
print('found 0x%X %s' % (hwnd, title))

wr = R()
u.GetWindowRect(hwnd, ctypes.byref(wr))
ww, hh = wr.r - wr.l, wr.b - wr.t
bi = BMIH()
bi.biSize = ctypes.sizeof(BMIH)
bi.biWidth, bi.biHeight = ww, -hh
bi.biPlanes, bi.biBitCount = 1, 32
bits = ctypes.c_void_p()
bmp = g.CreateDIBSection(0, ctypes.byref(bi), 0, ctypes.byref(bits), 0, 0)
mdc = g.CreateCompatibleDC(0)
g.SelectObject(mdc, bmp)
u.PrintWindow(hwnd, mdc, 1)

kids = []


def on_child(h, _):
    kids.append((h, u.GetDlgCtrlID(h)))
    return True


u.EnumChildWindows(hwnd, EP(on_child), 0)
for h, cid in kids:
    cr = R()
    u.GetWindowRect(h, ctypes.byref(cr))
    pt = R()
    pt.l, pt.t = cr.l, cr.t
    u.MapWindowPoints(None, hwnd, ctypes.byref(pt), 2)
    g.SetViewportOrgEx(mdc, pt.l, pt.t, None)
    u.SendMessageW(h, WM_PRINTCLIENT, mdc, PRF_CLIENT)
g.SetViewportOrgEx(mdc, 0, 0, None)

data = ctypes.string_at(bits.value, ww * hh * 4)
bi2 = BMIH()
bi2.biSize = ctypes.sizeof(BMIH)
bi2.biWidth, bi2.biHeight, bi2.biPlanes, bi2.biBitCount, bi2.biCompression = ww, -hh, 1, 32, 0
bi2.biSizeImage = ww * hh * 4
with open(OUT, 'wb') as f:
    f.write(b'BM')
    filesize = 14 + ctypes.sizeof(BMIH) + len(data)
    f.write(filesize.to_bytes(4, 'little'))
    f.write(b'\x00\x00\x00\x00')
    f.write((14 + ctypes.sizeof(BMIH)).to_bytes(4, 'little'))
    f.write(bytes(bi2))
    f.write(data)
print('bmp %s %dx%d kids=%d' % (OUT, ww, hh, len(kids)))
