# -*- coding: utf-8 -*-
"""把系统箭头、系统手型、我们的 grab 放在同一张图上（都是 32px），直观比较大小。"""
import ctypes
from ctypes import wintypes as wt
import struct
import zlib

CUR = r'G:\PyArx\X64\Release\res\grab.cur'
OUT = r'G:\PyArx\docs\_grab_size_cmp.png'
IMAGE_CURSOR = 2
LR_LOADFROMFILE = 0x10
IDC_ARROW = 32512
IDC_HAND = 32649


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [('biSize', wt.DWORD), ('biWidth', wt.LONG), ('biHeight', wt.LONG),
                ('biPlanes', wt.WORD), ('biBitCount', wt.WORD), ('biCompression', wt.DWORD),
                ('biSizeImage', wt.DWORD), ('biXPelsPerMeter', wt.LONG), ('biYPelsPerMeter', wt.LONG),
                ('biClrUsed', wt.DWORD), ('biClrImportant', wt.DWORD)]


u = ctypes.windll.user32
g = ctypes.windll.gdi32
u.LoadImageW.restype = wt.HANDLE
u.LoadImageW.argtypes = [wt.HINSTANCE, wt.LPCWSTR, wt.UINT, ctypes.c_int, ctypes.c_int, wt.UINT]
u.LoadCursorW.restype = wt.HANDLE
u.LoadCursorW.argtypes = [wt.HINSTANCE, ctypes.c_void_p]
u.DrawIconEx.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int, wt.HANDLE, ctypes.c_int, ctypes.c_int,
                         wt.UINT, wt.HANDLE, wt.UINT]
g.CreateDIBSection.restype = wt.HANDLE
g.CreateDIBSection.argtypes = [wt.HDC, ctypes.c_void_p, wt.UINT, ctypes.c_void_p, wt.HANDLE, wt.UINT]
g.CreateCompatibleDC.restype = wt.HANDLE
g.CreateCompatibleDC.argtypes = [wt.HDC]
g.SelectObject.restype = wt.HANDLE
g.SelectObject.argtypes = [wt.HDC, wt.HANDLE]
g.CreateSolidBrush.restype = wt.HANDLE
g.CreateSolidBrush.argtypes = [wt.COLORREF]
u.FillRect.argtypes = [wt.HDC, ctypes.c_void_p, wt.HANDLE]
g.DeleteObject.argtypes = [wt.HANDLE]
g.DeleteDC.argtypes = [wt.HDC]


def render(hcur, size, scale):
    bih = BITMAPINFOHEADER(40, size, -size, 1, 32, 0, size * size * 4, 0, 0, 0, 0)
    bits = ctypes.c_void_p()
    hbm = g.CreateDIBSection(0, ctypes.byref(bih), 0, ctypes.byref(bits), None, 0)
    mem = g.CreateCompatibleDC(0)
    g.SelectObject(mem, hbm)
    br = g.CreateSolidBrush(0x00E6E6E6)
    r = wt.RECT(0, 0, size, size)
    u.FillRect(mem, ctypes.byref(r), br)
    u.DrawIconEx(mem, 0, 0, hcur, size, size, 0, None, 0x0003)
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
    g.DeleteObject(br)
    g.DeleteObject(hbm)
    g.DeleteDC(mem)
    return rows


def png(path, w, h, rows):
    raw = bytearray()
    for y in range(h):
        raw += b'\x00' + rows[y]

    def ck(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF)

    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' +
                ck(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) +
                ck(b'IDAT', zlib.compress(bytes(raw), 9)) + ck(b'IEND', b''))


arrow = u.LoadCursorW(None, ctypes.c_void_p(IDC_ARROW))
hand = u.LoadCursorW(None, ctypes.c_void_p(IDC_HAND))
grab = u.LoadImageW(None, CUR, IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE)
print('arrow=%s hand=%s grab=%s' % (arrow, hand, grab))

size, scale = 32, 8
parts = [render(c, size, scale) for c in (arrow, hand, grab)]
W = len(parts[0][0]) // 3 * len(parts) + 10 * (len(parts) + 1)
H = size * scale + 20
sheet = [bytearray(b'\xeb\xeb\xeb' * W) for _ in range(H)]
x0 = 10
for rows in parts:
    pw = len(rows[0]) // 3
    for y, row in enumerate(rows):
        sheet[y + 10][x0 * 3:(x0 + pw) * 3] = row
    x0 += pw + 10
png(OUT, W, H, sheet)
print('written', OUT)
