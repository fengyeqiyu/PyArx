# -*- coding: utf-8 -*-
"""启动配电图应用，跑几帧后把窗口抓成 PNG（不移动真实鼠标）。"""
import ctypes
from ctypes import wintypes as wt
import os
import struct
import sys
import time
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
os.chdir(HERE)
os.environ.setdefault('PYARX_D2D_RT', 'software')

import pp_app  # noqa: E402
import PyMfcXaml as M  # noqa: E402

OUT = r'G:\PyArx\docs\_pp_run.png'

u = ctypes.windll.user32
g = ctypes.windll.gdi32
g.CreateCompatibleDC.restype = wt.HANDLE
g.CreateCompatibleDC.argtypes = [wt.HDC]
g.CreateCompatibleBitmap.restype = wt.HANDLE
g.CreateCompatibleBitmap.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int]
g.SelectObject.restype = wt.HANDLE
g.SelectObject.argtypes = [wt.HDC, wt.HANDLE]
u.PrintWindow.argtypes = [wt.HWND, wt.HDC, wt.UINT]
g.DeleteObject.argtypes = [wt.HANDLE]
g.DeleteDC.argtypes = [wt.HDC]
u.GetDC.restype = wt.HDC
u.ReleaseDC.argtypes = [wt.HWND, wt.HDC]
g.GetDIBits.argtypes = [wt.HDC, wt.HANDLE, wt.UINT, wt.UINT, ctypes.c_void_p, ctypes.c_void_p, wt.UINT]


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [('biSize', wt.DWORD), ('biWidth', wt.LONG), ('biHeight', wt.LONG),
                ('biPlanes', wt.WORD), ('biBitCount', wt.WORD), ('biCompression', wt.DWORD),
                ('biSizeImage', wt.DWORD), ('biXPelsPerMeter', wt.LONG), ('biYPelsPerMeter', wt.LONG),
                ('biClrUsed', wt.DWORD), ('biClrImportant', wt.DWORD)]


def png(path, w, h, rows):
    raw = bytearray()
    for y in range(h):
        raw += b'\x00' + rows[y]

    def ck(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF)

    data = (b'\x89PNG\r\n\x1a\n' +
            ck(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) +
            ck(b'IDAT', zlib.compress(bytes(raw), 9)) + ck(b'IEND', b''))
    with open(path, 'wb') as f:
        f.write(data)


def find_window(pid):
    best = []

    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(hwnd, lp):
        if not u.IsWindowVisible(hwnd):
            return True
        p = wt.DWORD()
        u.GetWindowThreadProcessId(hwnd, ctypes.byref(p))
        if p.value != pid:
            return True
        r = wt.RECT()
        u.GetClientRect(hwnd, ctypes.byref(r))
        w = r.right - r.left
        h = r.bottom - r.top
        if w > 400 and h > 300:
            best.append((w * h, hwnd, w, h))
        return True

    u.EnumWindows(cb, 0)
    best.sort(reverse=True)
    return best[0] if best else None


def shot(hwnd, w, h):
    sdc = u.GetDC(None)
    mdc = g.CreateCompatibleDC(sdc)
    bmp = g.CreateCompatibleBitmap(sdc, w, h)
    g.SelectObject(mdc, bmp)
    u.PrintWindow(hwnd, mdc, 0)
    bih = BITMAPINFOHEADER(40, w, -h, 1, 24, 0, w * h * 3, 0, 0, 0, 0)
    buf = (ctypes.c_ubyte * (w * h * 3))()
    g.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bih), 0)
    rows = []
    for y in range(h):
        row = bytearray()
        for x in range(w):
            i = (y * w + x) * 3
            row += bytes((buf[i + 2], buf[i + 1], buf[i]))
        rows.append(row)
    g.DeleteObject(bmp)
    g.DeleteDC(mdc)
    u.ReleaseDC(None, sdc)
    return rows


app = pp_app.App()
app.win.show()
app.show_editor()
for _ in range(40):
    M.pump(20)
print('canvas nodes =', len(getattr(app, 'nodes', [])))
time.sleep(0.6)
for _ in range(20):
    M.pump(10)

res = find_window(ctypes.windll.kernel32.GetCurrentProcessId())
print('window =', res)
if res:
    _, hwnd, w, h = res
    rows = shot(hwnd, w, h)
    png(OUT, w, h, rows)
    print('written', OUT, w, 'x', h)
app.win.close()
print('closed')
