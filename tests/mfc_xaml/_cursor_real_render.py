# -*- coding: utf-8 -*-
"""让 Windows 真正载入 grab.cur 并 DrawIcon 到位图，看最终屏幕像素（32 与 48 两个尺寸）。"""
import ctypes
from ctypes import wintypes as wt
import struct
import zlib

CUR = r'G:\PyArx\X64\Release\res\grab.cur'
OUT = r'G:\PyArx\docs\_grab_cursor_real.png'

IMAGE_CURSOR = 2
LR_LOADFROMFILE = 0x00000010
BI_RGB = 0
DIB_RGB_COLORS = 0


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [('biSize', wt.DWORD), ('biWidth', wt.LONG), ('biHeight', wt.LONG),
                ('biPlanes', wt.WORD), ('biBitCount', wt.WORD), ('biCompression', wt.DWORD),
                ('biSizeImage', wt.DWORD), ('biXPelsPerMeter', wt.LONG), ('biYPelsPerMeter', wt.LONG),
                ('biClrUsed', wt.DWORD), ('biClrImportant', wt.DWORD)]


class ICONINFO(ctypes.Structure):
    _fields_ = [('fIcon', wt.BOOL), ('xHotspot', wt.DWORD), ('yHotspot', wt.DWORD),
                ('hbmMask', wt.HBITMAP), ('hbmColor', wt.HBITMAP)]


u = ctypes.windll.user32
g = ctypes.windll.gdi32
u.LoadImageW.restype = wt.HANDLE
u.LoadImageW.argtypes = [wt.HINSTANCE, wt.LPCWSTR, wt.UINT, ctypes.c_int, ctypes.c_int, wt.UINT]
u.GetIconInfo.argtypes = [wt.HANDLE, ctypes.c_void_p]
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
u.FillRect.restype = ctypes.c_int
g.DeleteObject.argtypes = [wt.HANDLE]
g.DeleteDC.argtypes = [wt.HDC]


def save_png(path, w, h, rows):
    raw = bytearray()
    for y in range(h):
        raw += b'\x00' + bytes(rows[y])

    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF)

    png = (b'\x89PNG\r\n\x1a\n' +
           chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) +
           chunk(b'IDAT', zlib.compress(bytes(raw), 9)) +
           chunk(b'IEND', b''))
    with open(path, 'wb') as f:
        f.write(png)


def render(hcur, size, scale):
    bih = BITMAPINFOHEADER(40, size, -size, 1, 32, BI_RGB, size * size * 4, 0, 0, 0, 0)
    bits = ctypes.c_void_p()
    hbm = g.CreateDIBSection(0, ctypes.byref(bih), DIB_RGB_COLORS, ctypes.byref(bits), None, 0)
    mem = g.CreateCompatibleDC(0)
    g.SelectObject(mem, hbm)
    # 浅灰底
    br = g.CreateSolidBrush(0x00E6E6E6)
    r = wt.RECT(0, 0, size, size)
    u.FillRect(mem, ctypes.byref(r), br)
    # 画光标
    ok = u.DrawIconEx(mem, 0, 0, hcur, size, size, 0, None, 0x0003)
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
    return ok, rows, size * scale


hcur = u.LoadImageW(None, CUR, IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE)
print('LoadImageW ->', hcur)
if not hcur:
    raise SystemExit('load failed')
ii = ICONINFO()
u.GetIconInfo(hcur, ctypes.byref(ii))
print('hotspot =', ii.xHotspot, ii.yHotspot)

parts = []
for size in (32, 48):
    ok, rows, dim = render(hcur, size, 8)
    parts.append(rows)
    print('size %d DrawIconEx ok=%d' % (size, ok))

W = sum(len(p[0]) // 3 for p in parts) + 20
H = max(len(p) for p in parts) + 20
sheet = [bytearray(b'\xeb\xeb\xeb' * W) for _ in range(H)]
x0 = 10
for rows, pw in zip(parts, [len(p[0]) // 3 for p in parts]):
    for y, row in enumerate(rows):
        sheet[y + 10][x0 * 3:(x0 + pw) * 3] = row
    x0 += pw + 10
save_png(OUT, W, H, sheet)
print('written', OUT)
