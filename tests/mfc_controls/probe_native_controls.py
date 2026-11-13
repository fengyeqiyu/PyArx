# -*- coding: utf-8 -*-
"""对照实验：同样的消息序列直接发给纯 Win32 原生控件，看它们是否可见/可用。"""
import ctypes
import os
import sys
import time
from ctypes import wintypes

user32 = ctypes.windll.user32
gdi32 = ctypes.windll.gdi32
kernel32 = ctypes.windll.kernel32
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, 'native_probe.log')

PBM_SETPOS = 0x0402
TBM_SETPOS = 0x0405
PBM_GETPOS = 0x0408
TBM_GETPOS = 0x0400
BM_SETCHECK = 0x00F1
BM_GETCHECK = 0x00F0
DTM_SETSYSTEMTIME = 0x1001
DTM_GETSYSTEMTIME = 0x1002
GDT_VALID = 0
SRCCOPY = 0x00CC0020
DIB_RGB_COLORS = 0
SW_SHOW = 5


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [('biSize', wintypes.DWORD), ('biWidth', wintypes.LONG),
                ('biHeight', wintypes.LONG), ('biPlanes', wintypes.WORD),
                ('biBitCount', wintypes.WORD), ('biCompression', wintypes.DWORD),
                ('biSizeImage', wintypes.DWORD), ('biXPelsPerMeter', wintypes.LONG),
                ('biYPelsPerMeter', wintypes.LONG), ('biClrUsed', wintypes.DWORD),
                ('biClrImportant', wintypes.DWORD)]


class SYSTEMTIME(ctypes.Structure):
    _fields_ = [('wYear', wintypes.WORD), ('wMonth', wintypes.WORD),
                ('wDayOfWeek', wintypes.WORD), ('wDay', wintypes.WORD),
                ('wHour', wintypes.WORD), ('wMinute', wintypes.WORD),
                ('wSecond', wintypes.WORD), ('wMilliseconds', wintypes.WORD)]


kernel32.GetModuleHandleW.restype = wintypes.HMODULE
kernel32.GetModuleHandleW.argtypes = [wintypes.LPCWSTR]
user32.CreateWindowExW.restype = wintypes.HWND
user32.CreateWindowExW.argtypes = [wintypes.DWORD, wintypes.LPCWSTR, wintypes.LPCWSTR,
                                   wintypes.DWORD, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                                   ctypes.c_int, wintypes.HWND, wintypes.HMENU, wintypes.HINSTANCE,
                                   wintypes.LPVOID]
user32.GetDC.restype = wintypes.HDC
user32.GetDC.argtypes = [wintypes.HWND]
user32.GetClientRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
user32.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
user32.SendMessageW.restype = wintypes.LPARAM
user32.UpdateWindow.argtypes = [wintypes.HWND]
gdi32.CreateCompatibleDC.restype = wintypes.HDC
gdi32.CreateCompatibleDC.argtypes = [wintypes.HDC]
gdi32.CreateCompatibleBitmap.restype = wintypes.HBITMAP
gdi32.CreateCompatibleBitmap.argtypes = [wintypes.HDC, ctypes.c_int, ctypes.c_int]
gdi32.SelectObject.argtypes = [wintypes.HDC, wintypes.HGDIOBJ]
gdi32.BitBlt.argtypes = [wintypes.HDC, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                         wintypes.HDC, ctypes.c_int, ctypes.c_int, wintypes.DWORD]
gdi32.GetDIBits.argtypes = [wintypes.HDC, wintypes.HBITMAP, wintypes.UINT, wintypes.UINT,
                            ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT]
gdi32.DeleteDC.argtypes = [wintypes.HDC]
gdi32.DeleteObject.argtypes = [wintypes.HGDIOBJ]
user32.ReleaseDC.argtypes = [wintypes.HWND, wintypes.HDC]

lines = []


def out(t):
    lines.append(t)
    print(t, flush=True)


def snapshot(hwnd):
    r = wintypes.RECT()
    user32.GetClientRect(hwnd, ctypes.byref(r))
    w, h = r.right, r.bottom
    if w <= 0 or h <= 0:
        return None
    hdc = user32.GetDC(hwnd)
    mem = gdi32.CreateCompatibleDC(hdc)
    bmp = gdi32.CreateCompatibleBitmap(hdc, w, h)
    old = gdi32.SelectObject(mem, wintypes.HGDIOBJ(bmp))
    gdi32.BitBlt(mem, 0, 0, w, h, hdc, 0, 0, SRCCOPY)
    bi = BITMAPINFOHEADER()
    bi.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    bi.biWidth = w
    bi.biHeight = -h
    bi.biPlanes = 1
    bi.biBitCount = 32
    buf = (ctypes.c_ubyte * (w * h * 4))()
    gdi32.GetDIBits(mem, wintypes.HBITMAP(bmp), 0, h, buf, ctypes.byref(bi), DIB_RGB_COLORS)
    gdi32.SelectObject(mem, wintypes.HGDIOBJ(old))
    gdi32.DeleteObject(wintypes.HGDIOBJ(bmp))
    gdi32.DeleteDC(wintypes.HDC(mem))
    user32.ReleaseDC(hwnd, hdc)
    return w, h, bytes(buf)


def diff(a, b):
    if not a or not b:
        return None
    pa, pb = a[2], b[2]
    changed = 0
    for i in range(0, len(pa), 4):
        if abs(pa[i] - pb[i]) > 8 or abs(pa[i + 1] - pb[i + 1]) > 8 or abs(pa[i + 2] - pb[i + 2]) > 8:
            changed += 1
    return changed / float(a[0] * a[1])


def main():
    hinst = kernel32.GetModuleHandleW(None)
    parent = user32.CreateWindowExW(0, 'STATIC', 'NativeProbe', 0x00CF0000, 200, 200, 520, 380,
                                    0, 0, hinst, None)
    user32.ShowWindow(parent, SW_SHOW)
    user32.UpdateWindow(parent)

    def child(cls, text, style, x, y, w, h):
        hw = user32.CreateWindowExW(0, cls, text, 0x50000000 | style, x, y, w, h,
                                    parent, 0, hinst, None)
        user32.ShowWindow(hw, SW_SHOW)
        user32.UpdateWindow(hw)
        return hw

    # 原生 ProgressBar
    pb = child('msctls_progress32', None, 0x00000001, 20, 20, 240, 26)
    user32.SendMessageW(pb, 0x0406, 0, 100)               # PBM_SETRANGE32
    user32.SendMessageW(pb, 0x0402, 20, 0)                 # PBM_SETPOS 20
    user32.UpdateWindow(pb)
    time.sleep(0.15)
    a = snapshot(pb)
    user32.SendMessageW(pb, PBM_SETPOS, 90, 0)
    user32.UpdateWindow(pb)
    time.sleep(0.15)
    b = snapshot(pb)
    out('原生 ProgressBar 20->90 像素变化 %.2f%%' % ((diff(a, b) or 0) * 100))

    # 原生 Trackbar
    tb = child('msctls_trackbar32', None, 0x00000001, 20, 70, 240, 40)
    user32.SendMessageW(tb, TBM_SETPOS, 1, 10)
    user32.UpdateWindow(tb)
    time.sleep(0.15)
    a = snapshot(tb)
    user32.SendMessageW(tb, TBM_SETPOS, 1, 80)
    user32.UpdateWindow(tb)
    time.sleep(0.15)
    b = snapshot(tb)
    out('原生 Slider 10->80 像素变化 %.2f%%' % ((diff(a, b) or 0) * 100))

    # 原生 CheckBox
    cb = child('Button', '复选', 0x00000003, 20, 130, 200, 30)   # BS_AUTOCHECKBOX
    user32.SendMessageW(cb, BM_SETCHECK, 0, 0)
    user32.UpdateWindow(cb)
    time.sleep(0.15)
    a = snapshot(cb)
    user32.SendMessageW(cb, BM_SETCHECK, 1, 0)
    user32.UpdateWindow(cb)
    time.sleep(0.15)
    b = snapshot(cb)
    out('原生 CheckBox 未勾->勾 像素变化 %.2f%%（BM_GETCHECK=%d）'
        % ((diff(a, b) or 0) * 100, user32.SendMessageW(cb, BM_GETCHECK, 0, 0)))

    # 原生 DatePicker
    dp = child('SysDateTimePick32', None, 0x00000001, 20, 180, 240, 32)
    st = SYSTEMTIME(2027, 3, 0, 15, 9, 30, 0, 0)
    r_set = user32.SendMessageW(dp, DTM_SETSYSTEMTIME, GDT_VALID, ctypes.addressof(st))
    got = SYSTEMTIME()
    r_get = user32.SendMessageW(dp, DTM_GETSYSTEMTIME, 0, ctypes.addressof(got))
    out('原生 DatePicker SET=%d GET=%d -> %d-%d-%d'
        % (r_set, r_get, got.wYear, got.wMonth, got.wDay))

    with open(OUT, 'w', encoding='utf-8') as fh:
        fh.write('\n'.join(lines) + '\n')
    user32.DestroyWindow(parent)


if __name__ == '__main__':
    sys.exit(main())
