# -*- coding: utf-8 -*-
"""_shot_flick.py — 抓 Flickable 的屏幕真像素，验"越界内容被视口裁掉"。

三拍：初始（顶行贴视口上沿）/ 向下拖 120（内容下移，上沿露出空白，底部两行被裁）/ 回弹后。
"""
import ctypes
import os
from ctypes import wintypes

import PyMfcXaml as p
from demo_flickable import CONTENT_H, build
from _verify_flickable import BMIH, R, drag, find_child_by_id, lparam, release

u = ctypes.windll.user32
g = ctypes.windll.gdi32
u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.GetDC.argtypes = [ctypes.c_void_p]
u.GetDC.restype = ctypes.c_void_p
u.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
g.CreateCompatibleDC.argtypes = [ctypes.c_void_p]
g.CreateCompatibleDC.restype = ctypes.c_void_p
g.CreateDIBSection.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD,
                               ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD]
g.CreateDIBSection.restype = ctypes.c_void_p
g.SelectObject.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
g.SelectObject.restype = ctypes.c_void_p
g.BitBlt.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                     ctypes.c_void_p, ctypes.c_int, ctypes.c_int, wintypes.DWORD]


def shot(hwnd, out):
    wr = R()
    u.GetWindowRect(hwnd, ctypes.byref(wr))
    ww, hh = wr.r - wr.l, wr.b - wr.t
    bi = BMIH()
    bi.biSize = ctypes.sizeof(BMIH)
    bi.biWidth, bi.biHeight, bi.biPlanes, bi.biBitCount = ww, -hh, 1, 32
    bits = ctypes.c_void_p()
    bmp = g.CreateDIBSection(0, ctypes.byref(bi), 0, ctypes.byref(bits), 0, 0)
    src = u.GetDC(0)
    mdc = g.CreateCompatibleDC(0)
    g.SelectObject(mdc, bmp)
    g.BitBlt(mdc, 0, 0, ww, hh, src, wr.l, wr.t, 0x00CC0020)
    data = ctypes.string_at(bits.value, ww * hh * 4)
    bi2 = BMIH()
    bi2.biSize = ctypes.sizeof(BMIH)
    bi2.biWidth, bi2.biHeight, bi2.biPlanes, bi2.biBitCount, bi2.biCompression = ww, -hh, 1, 32, 0
    bi2.biSizeImage = ww * hh * 4
    with open(out, 'wb') as f:
        f.write(b'BM')
        f.write((14 + ctypes.sizeof(BMIH) + len(data)).to_bytes(4, 'little'))
        f.write(b'\x00\x00\x00\x00')
        f.write((14 + ctypes.sizeof(BMIH)).to_bytes(4, 'little'))
        f.write(bytes(bi2))
        f.write(data)
    print('shot %s %dx%d' % (out, ww, hh))


path = build()
names = {}
w = p.guiload(path, names=names)
fid = names['fl']
kids = [names['r%d' % i] for i in range(8)]
w.setFlickContent(fid, kids, 340, CONTENT_H)
w.show()
p.pump(300)
hv = find_child_by_id(w, fid)
out_dir = r'G:\PyArx\docs'
shot(w.hwnd, os.path.join(out_dir, '_fl_s1_top.bmp'))

drag(hv, [(120, 90), (120, 130), (120, 170), (120, 200)], 12)
p.pump(80)
shot(w.hwnd, os.path.join(out_dir, '_fl_s2_scrolled.bmp'))
release(hv, (120, 200))
p.pump(800)
shot(w.hwnd, os.path.join(out_dir, '_fl_s3_back.bmp'))
print('offset after drag/back = %d,%d' % (w.getFlickOffsetX(fid), w.getFlickOffsetY(fid)))
w.close()
