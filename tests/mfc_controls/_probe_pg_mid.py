# -*- coding: utf-8 -*-
"""_probe_pg_mid.py — 切页动画中途连拍（不用 savePng，因为它会泵 600ms 消息把动画跑完）。

直接 BitBlt 真屏幕 → 32bpp BMP（顶朝下），PowerShell 再转 PNG。
"""
import ctypes
import os
import struct

import PyMfcXaml as p
from demo_page_anim import build, PageAnim

OUT = r'G:\PyArx\docs'
u = ctypes.windll.user32
g = ctypes.windll.gdi32
u.SetForegroundWindow.argtypes = [ctypes.c_void_p]
u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
g.CreateCompatibleDC.argtypes = [ctypes.c_void_p]
g.CreateCompatibleDC.restype = ctypes.c_void_p
g.CreateCompatibleBitmap.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
g.CreateCompatibleBitmap.restype = ctypes.c_void_p
g.SelectObject.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
g.BitBlt.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                     ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_uint]
g.GetDIBits.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint, ctypes.c_uint,
                        ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint]
u.GetDC.argtypes = [ctypes.c_void_p]
u.GetDC.restype = ctypes.c_void_p
u.ReleaseDC.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
g.DeleteObject.argtypes = [ctypes.c_void_p]
g.DeleteDC.argtypes = [ctypes.c_void_p]


class R(ctypes.Structure):
    _fields_ = [('l', ctypes.c_long), ('t', ctypes.c_long),
                ('r', ctypes.c_long), ('b', ctypes.c_long)]


class BIH(ctypes.Structure):
    _fields_ = [('biSize', ctypes.c_ulong), ('biWidth', ctypes.c_long),
                ('biHeight', ctypes.c_long), ('biPlanes', ctypes.c_ushort),
                ('biBitCount', ctypes.c_ushort), ('biCompression', ctypes.c_ulong),
                ('biSizeImage', ctypes.c_ulong), ('biXPelsPerMeter', ctypes.c_long),
                ('biYPelsPerMeter', ctypes.c_long), ('biClrUsed', ctypes.c_ulong),
                ('biClrImportant', ctypes.c_ulong)]


def shot(hwnd, path):
    r = R()
    u.GetWindowRect(ctypes.c_void_p(hwnd), ctypes.byref(r))
    w, h = r.r - r.l, r.b - r.t
    sdc = u.GetDC(None)
    mdc = g.CreateCompatibleDC(sdc)
    bmp = g.CreateCompatibleBitmap(sdc, w, h)
    g.SelectObject(mdc, bmp)
    g.BitBlt(mdc, 0, 0, w, h, sdc, r.l, r.t, 0x40000000 | 0x00CC0020)
    bih = BIH(40, w, -h, 1, 32, 0, w * h * 4, 2835, 2835, 0, 0)
    buf = ctypes.create_string_buffer(w * h * 4)
    g.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bih), 0)
    with open(path, 'wb') as f:
        f.write(struct.pack('<2sIHHI', b'BM', 54 + w * h * 4, 0, 0, 54))
        f.write(bytes(bih))
        f.write(buf.raw)
    g.DeleteObject(bmp)
    g.DeleteDC(mdc)
    u.ReleaseDC(None, sdc)


names = {}
w = p.guiload(build(), names=names)
w.show()
p.pump(300)
sv = names['sv']
w.addPage(sv, [names['s1'], names['p1t'], names['p1b'], names['p1p']])
w.addPage(sv, [names['s2'], names['p2t'], names['p2s']])
w.addPage(sv, [names['s3'], names['p3t'], names['p3d']])
pa = PageAnim(w, names)
pa.tick()
p.pump(900)

hwnd_top = w.hwnd
w.animatePage(sv, 1, 260)
for i in range(5):
    p.pump(40)
    shot(w.hwnd, os.path.join(OUT, '_pgs_%d.bmp' % (i + 1)))
p.pump(400)
shot(w.hwnd, os.path.join(OUT, '_pgs_end.bmp'))
print('captured', flush=True)
w.close()
p.pump(200)
p.shutdown()
