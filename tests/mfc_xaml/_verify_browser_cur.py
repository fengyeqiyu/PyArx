# -*- coding: utf-8 -*-
"""核对从浏览器 HCURSOR 抓出来的 .cur：尺寸/热区/LoadImage 是否可用，并出 8 倍预览。"""
import ctypes
import os
import struct
from ctypes import wintypes as wt

D = r'G:\PyArx\docs\_browser_cursors'
OUT = r'G:\PyArx\docs\_browser_cursors\_verify.png'
u = ctypes.windll.user32
g = ctypes.windll.gdi32
u.LoadImageW.restype = wt.HANDLE
u.LoadImageW.argtypes = [wt.HINSTANCE, wt.LPCWSTR, wt.UINT, ctypes.c_int, ctypes.c_int, wt.UINT]
u.GetIconInfo.argtypes = [wt.HANDLE, ctypes.c_void_p]
u.DrawIconEx.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int, wt.HANDLE, ctypes.c_int, ctypes.c_int,
                         wt.UINT, wt.HANDLE, wt.UINT]


class ICONINFO(ctypes.Structure):
    _fields_ = [('fIcon', wt.BOOL), ('xHotspot', wt.DWORD), ('yHotspot', wt.DWORD),
                ('hbmMask', wt.HBITMAP), ('hbmColor', wt.HBITMAP)]


class BIH(ctypes.Structure):
    _fields_ = [('biSize', wt.DWORD), ('biWidth', wt.LONG), ('biHeight', wt.LONG),
                ('biPlanes', wt.WORD), ('biBitCount', wt.WORD), ('biCompression', wt.DWORD),
                ('biSizeImage', wt.DWORD), ('biXPelsPerMeter', wt.LONG), ('biYPelsPerMeter', wt.LONG),
                ('biClrUsed', wt.DWORD), ('biClrImportant', wt.DWORD)]


g.CreateDIBSection.restype = wt.HANDLE
g.CreateDIBSection.argtypes = [wt.HDC, ctypes.c_void_p, wt.UINT, ctypes.c_void_p, wt.HANDLE, wt.UINT]
g.CreateCompatibleDC.restype = wt.HANDLE
g.CreateCompatibleDC.argtypes = [wt.HDC]
g.SelectObject.argtypes = [wt.HDC, wt.HANDLE]
g.SelectObject.restype = wt.HANDLE
g.CreateSolidBrush.restype = wt.HANDLE
g.CreateSolidBrush.argtypes = [wt.COLORREF]
u.FillRect.argtypes = [wt.HDC, ctypes.c_void_p, wt.HANDLE]


def png(path, w, h, rows):
    import zlib
    raw = b''.join(b'\x00' + bytes(r) for r in rows)
    ck = lambda t, d: struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xFFFFFFFF)
    data = (b'\x89PNG\r\n\x1a\n' + ck(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) +
            ck(b'IDAT', zlib.compress(raw, 9)) + ck(b'IEND', b''))
    open(path, 'wb').write(data)


def render(hc, size, scale):
    bih = BIH(40, size, -size, 1, 32, 0, size * size * 4, 0, 0, 0, 0)
    bits = ctypes.c_void_p()
    hbm = g.CreateDIBSection(0, ctypes.byref(bih), 0, ctypes.byref(bits), None, 0)
    mem = g.CreateCompatibleDC(0)
    g.SelectObject(mem, hbm)
    br = g.CreateSolidBrush(0x00E6E6E6)
    r = wt.RECT(0, 0, size, size)
    u.FillRect(mem, ctypes.byref(r), br)
    u.DrawIconEx(mem, 0, 0, hc, size, size, 0, None, 3)
    buf = (ctypes.c_ubyte * (size * size * 4)).from_address(bits.value)
    rows = []
    for y in range(size * scale):
        sy = y // scale
        row = bytearray()
        for x in range(size * scale):
            sx = x // scale
            i = (sy * size + sx) * 4
            row += bytes((buf[i + 2], buf[i + 1], buf[i]))
        rows.append(row)
    return rows


names = ['grab_open.cur', 'grab_fist.cur']
cells = []
for n in names:
    p = os.path.join(D, n)
    if not os.path.exists(p):
        print('missing', n)
        continue
    raw = open(p, 'rb').read()
    cnt = struct.unpack('<H', raw[4:6])[0]
    w, h, _, _, hx, hy, sz, off = struct.unpack('<BBBBHHII', raw[6:22])
    print('%s bytes=%d entries=%d w=%d h=%d hotspot=(%d,%d)' % (n, len(raw), cnt, w or 256, h or 256, hx, hy))
    hc = u.LoadImageW(None, p, 2, 0, 0, 0x10)
    ii = ICONINFO()
    u.GetIconInfo(hc, ctypes.byref(ii))
    print('  LoadImage=%s  GetIconInfo hotspot=(%d,%d)' % (hc, ii.xHotspot, ii.yHotspot))
    cells.append(render(hc, 32, 8))

cell = 32 * 8
W = cell * len(cells) + 10 * (len(cells) + 1)
H = cell + 20
sheet = [bytearray(b'\xff\xff\xff' * W) for _ in range(H)]
x0 = 10
for rows in cells:
    for y, row in enumerate(rows):
        sheet[y + 10][x0 * 3:(x0 + cell) * 3] = row
    x0 += cell + 10
png(OUT, W, H, sheet)
print('sheet ->', OUT)
