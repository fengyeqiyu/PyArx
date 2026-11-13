# -*- coding: utf-8 -*-
"""_probe_viewer_shot.py — 抓预览器整窗（预览器里 Level 生效的取证图）。"""
import ctypes
import ctypes.wintypes as wt
import os
import struct
import subprocess
import sys
import time

u = ctypes.windll.user32
g = ctypes.windll.gdi32
u.SetProcessDPIAware()
u.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u.EnumWindows.argtypes = [ctypes.c_void_p, wt.LPARAM]
u.GetWindowTextW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]

VIEWER = r'G:\PyArx\X64\Release\Viewer.exe'
XAML = r'G:\PyArx\tests\mfc_controls\level_tab_viewer.xaml'
OUT = r'G:\PyArx\docs'


def find(title):
    found = []

    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(h, _):
        b = ctypes.create_unicode_buffer(512)
        u.GetWindowTextW(h, b, 512)
        if title in b.value:
            found.append(h)
            return 0
        return 1

    u.EnumWindows(ctypes.cast(cb, ctypes.c_void_p), 0)
    return found[0] if found else None


class BIH(ctypes.Structure):
    _fields_ = [('biSize', wt.DWORD), ('biWidth', ctypes.c_long),
                ('biHeight', ctypes.c_long), ('biPlanes', wt.WORD),
                ('biBitCount', wt.WORD), ('biCompression', wt.DWORD),
                ('biSizeImage', wt.DWORD), ('biXPelsPerMeter', ctypes.c_long),
                ('biYPelsPerMeter', ctypes.c_long), ('biClrUsed', wt.DWORD),
                ('biClrImportant', wt.DWORD)]


def grab(x, y, w, h, bmp_path):
    hdc = u.GetDC(0)
    mdc = g.CreateCompatibleDC(hdc)
    bmp = g.CreateCompatibleBitmap(hdc, w, h)
    g.SelectObject(mdc, bmp)
    g.BitBlt(mdc, 0, 0, w, h, hdc, x, y, 0x00CC0020)
    bi = BIH()
    bi.biSize = ctypes.sizeof(BIH)
    bi.biWidth, bi.biHeight, bi.biPlanes, bi.biBitCount = w, -h, 1, 24
    size = ((w * 3 + 3) // 4) * 4 * h
    bits = ctypes.create_string_buffer(size)
    g.GetDIBits(mdc, bmp, 0, h, bits, ctypes.byref(bi), 0)
    fh = struct.pack('<IHH', 14 + 40 + size, 0, 0) + struct.pack('<I', 14 + 40)
    ih = struct.pack('<IiiHHIIiiII', 40, w, -h, 1, 24, 0, size, 0, 0, 0, 0)
    with open(bmp_path, 'wb') as f:
        f.write(b'BM' + fh + ih + bits.raw)
    g.DeleteObject(bmp)
    g.DeleteDC(mdc)
    u.ReleaseDC(0, hdc)


if __name__ == '__main__':
    p = subprocess.Popen([VIEWER, XAML], cwd=os.path.dirname(VIEWER))
    h = None
    for _ in range(30):
        time.sleep(0.5)
        h = find('预览器 · 层次与 Tab 序号')
        if h:
            break
    if not h:
        print('未找到预览器窗口')
        p.terminate()
        sys.exit(1)
    u.SetForegroundWindow(h)
    time.sleep(1.0)
    r = wt.RECT()
    u.GetWindowRect(h, ctypes.byref(r))
    grab(r.left, r.top, r.right - r.left, r.bottom - r.top, os.path.join(OUT, '_vo_win.bmp'))
    print('saved _vo_win.bmp', (r.left, r.top, r.right, r.bottom))
    p.terminate()
