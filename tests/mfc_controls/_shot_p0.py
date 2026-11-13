# -*- coding: utf-8 -*-
"""把 P0 验收窗整窗合成成一张位图（父窗口 PrintWindow + 每个子窗口 WM_PRINTCLIENT 平移叠上去）。

本自绘 UI 的内容不进窗口表面：PrintWindow 只拿到父窗口底板，QCanvas 的内容得靠
逐个子窗口 WM_PRINTCLIENT 画到同一个内存 DC（用 SetViewportOrgEx 平移到各自位置）。
输出 BMP（32bpp），再由 PowerShell 的 System.Drawing 转成 PNG。
"""
import ctypes
import os
from ctypes import wintypes

import PyMfcXaml as p

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_BMP = os.path.join(HERE, '_p0_shot.bmp')

u = ctypes.windll.user32
g = ctypes.windll.gdi32

WM_PRINTCLIENT = 0x0318
PRF_CLIENT = 0x00000004

u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
u.GetDlgCtrlID.argtypes = [ctypes.c_void_p]
u.GetDlgCtrlID.restype = ctypes.c_int
u.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
u.SendMessageW.restype = wintypes.LPARAM
u.PrintWindow.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT]
u.PrintWindow.restype = wintypes.BOOL
u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
g.SetViewportOrgEx.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_void_p]
u.MapWindowPoints.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT]

g.CreateCompatibleDC.argtypes = [ctypes.c_void_p]
g.CreateCompatibleDC.restype = ctypes.c_void_p
g.CreateDIBSection.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD,
                               ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD]
g.CreateDIBSection.restype = ctypes.c_void_p
g.SelectObject.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
g.SelectObject.restype = ctypes.c_void_p


class R(ctypes.Structure):
    _fields_ = [('l', ctypes.c_long), ('t', ctypes.c_long), ('r', ctypes.c_long), ('b', ctypes.c_long)]


class BMIH(ctypes.Structure):
    _fields_ = [('biSize', wintypes.DWORD), ('biWidth', ctypes.c_long), ('biHeight', ctypes.c_long),
                ('biPlanes', wintypes.WORD), ('biBitCount', wintypes.WORD),
                ('biCompression', wintypes.DWORD), ('biSizeImage', wintypes.DWORD),
                ('biXPelsPerMeter', ctypes.c_long), ('biYPelsPerMeter', ctypes.c_long),
                ('biClrUsed', wintypes.DWORD), ('biClrImportant', wintypes.DWORD)]


names = {}
w = p.guiload(os.path.join(HERE, '_p0_shot.xaml'), names=names)
w.show()
p.pump(700)

wr = R()
u.GetWindowRect(w.hwnd, ctypes.byref(wr))
ww, hh = wr.r - wr.l, wr.b - wr.t

bi = BMIH()
bi.biSize = ctypes.sizeof(BMIH)
bi.biWidth, bi.biHeight = ww, -hh
bi.biPlanes, bi.biBitCount = 1, 32
bits = ctypes.c_void_p()
bmp = g.CreateDIBSection(0, ctypes.byref(bi), 0, ctypes.byref(bits), 0, 0)
mdc = g.CreateCompatibleDC(0)
g.SelectObject(mdc, bmp)

u.PrintWindow(w.hwnd, mdc, 1)   # PW_CLIENTONLY

kids = []
EP = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


def cb(h, _):
    kids.append((h, u.GetDlgCtrlID(h)))
    return True
u.EnumChildWindows(w.hwnd, EP(cb), 0)

for h, cid in kids:
    cr = R()
    u.GetWindowRect(h, ctypes.byref(cr))
    pt = R()
    pt.l, pt.t = cr.l, cr.t
    u.MapWindowPoints(None, w.hwnd, ctypes.byref(pt), 2)
    g.SetViewportOrgEx(mdc, pt.l, pt.t, None)
    u.SendMessageW(h, WM_PRINTCLIENT, mdc, PRF_CLIENT)
g.SetViewportOrgEx(mdc, 0, 0, None)

# 直接按 DIB 的内存块写一个 32bpp BMP 文件
data = ctypes.string_at(bits.value, ww * hh * 4)
bi2 = BMIH()
bi2.biSize = ctypes.sizeof(BMIH)
bi2.biWidth, bi2.biHeight, bi2.biPlanes, bi2.biBitCount, bi2.biCompression = ww, -hh, 1, 32, 0
bi2.biSizeImage = ww * hh * 4
with open(OUT_BMP, 'wb') as f:
    f.write(b'BM')
    filesize = 14 + ctypes.sizeof(BMIH) + len(data)
    f.write(filesize.to_bytes(4, 'little'))
    f.write(b'\x00\x00\x00\x00')
    f.write((14 + ctypes.sizeof(BMIH)).to_bytes(4, 'little'))
    f.write(bytes(bi2))
    f.write(data)
print('bmp', OUT_BMP, ww, hh)

p.pump(200)
w.close()
p.pump(200)
p.shutdown()
