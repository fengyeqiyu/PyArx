# -*- coding: utf-8 -*-
"""S1 表面路径布局诊断：枚举宿主全部子窗的 id/类/窗口矩形/客户区矩形，对比 =1 与 =0。
用法: PYARX_SURFACE=1 python _diag_surface_layout.py
"""
import ctypes
import ctypes.wintypes as wt
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')

import PyMfcXaml as M
import pp_app

u32 = ctypes.WinDLL('user32', use_last_error=True)
GW_CHILD, GW_HWNDNEXT, GWL_ID = 5, 2, -12
u32.GetWindow.argtypes = [wt.HWND, wt.UINT]
u32.GetWindow.restype = wt.HWND
u32.GetWindowLongPtrW.argtypes = [wt.HWND, ctypes.c_int]
u32.GetWindowLongPtrW.restype = ctypes.c_ssize_t
u32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u32.GetClientRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u32.GetClassNameW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
u32.SetForegroundWindow.argtypes = [wt.HWND]
g32 = ctypes.WinDLL('gdi32', use_last_error=True)

sys.path.insert(0, r'G:\PyArx\pylibs')
import numpy as np


class _BIH(ctypes.Structure):
    _fields_ = [("biSize", wt.UINT), ("biWidth", ctypes.c_long), ("biHeight", ctypes.c_long),
                ("biPlanes", wt.WORD), ("biBitCount", wt.WORD), ("biCompression", wt.DWORD),
                ("biSizeImage", wt.DWORD), ("biXPelsPerMeter", ctypes.c_long),
                ("biYPelsPerMeter", ctypes.c_long), ("biClrUsed", wt.DWORD),
                ("biClrImportant", wt.DWORD)]


class Grabber:
    def __init__(self):
        self.screen = u32.GetDC(0)

    def rect(self, hwnd):
        r = wt.RECT(); u32.GetWindowRect(hwnd, ctypes.byref(r)); return r

    def frame(self, rect):
        w = rect.right - rect.left; h = rect.bottom - rect.top
        if w <= 0 or h <= 0: return None
        bi = _BIH(); bi.biSize = ctypes.sizeof(_BIH); bi.biWidth = w; bi.biHeight = -h
        bi.biPlanes = 1; bi.biBitCount = 32; bi.biCompression = 0
        bits = ctypes.c_void_p(); dib = g32.CreateDIBSection(self.screen, ctypes.byref(bi), 0,
                                                              ctypes.byref(bits), None, 0)
        if dib is None or bits.value is None: return None
        mem = g32.CreateCompatibleDC(self.screen); old = g32.SelectObject(mem, dib)
        g32.BitBlt(mem, 0, 0, w, h, self.screen, rect.left, rect.top, 0x00CC0020)
        raw = ctypes.string_at(bits, w * h * 4)
        arr = np.frombuffer(raw, dtype=np.uint8).reshape(h, w, 4).copy()
        g32.SelectObject(mem, old); g32.DeleteObject(dib); g32.DeleteDC(mem)
        return arr


mode = os.environ.get('PYARX_SURFACE', '0')
print('=== PYARX_SURFACE=%s D2D_RT=%s ===' % (mode, os.environ.get('PYARX_D2D_RT', '-')), flush=True)

app = pp_app.App()
app.win.show()
M.pump(80)
app.showEditor()
M.pump(250)
hwnd = app.win.hwnd
u32.SetForegroundWindow(hwnd)
M.pump(120)

wr = wt.RECT(); cr = wt.RECT()
u32.GetWindowRect(hwnd, ctypes.byref(wr))
u32.GetClientRect(hwnd, ctypes.byref(cr))
print('HOST hwnd=%s win=(%d,%d,%d,%d) %dx%d client=%dx%d'
      % (hwnd, wr.left, wr.top, wr.right, wr.bottom,
         wr.right - wr.left, wr.bottom - wr.top, cr.right, cr.bottom), flush=True)

rows = []


def walk(h, depth=0):
    while h:
        cid = int(u32.GetWindowLongPtrW(h, GWL_ID))
        buf = ctypes.create_unicode_buffer(64)
        u32.GetClassNameW(h, buf, 64)
        w = wt.RECT(); c = wt.RECT()
        u32.GetWindowRect(h, ctypes.byref(w))
        u32.GetClientRect(h, ctypes.byref(c))
        rows.append((cid, buf.value,
                     (w.left, w.top, w.right, w.bottom),
                     (c.left, c.top, c.right, c.bottom), depth))
        walk(u32.GetWindow(h, GW_CHILD), depth + 1)
        h = u32.GetWindow(h, GW_HWNDNEXT)


walk(u32.GetWindow(hwnd, GW_CHILD))
rows.sort(key=lambda r: (r[0] < 0, r[0]))
for cid, cls, w, c, depth in rows:
    print('id=%-5d depth=%d cls=%-16s win=(%4d,%4d,%4d,%4d) %4dx%-4d client=%4dx%-4d'
          % (cid, depth, cls, w[0], w[1], w[2], w[3],
             w[2] - w[0], w[3] - w[1], c[2], c[3]), flush=True)

for name, cid in [('CV', pp_app.CV), ('MINI', pp_app.MINI)]:
    try:
        zoom, ox, oy = app.win.getPlanView(cid)
        print('VIEW %-5s id=%d zoom=%.3f ox=%d oy=%d' % (name, cid, zoom, ox, oy), flush=True)
    except Exception as e:
        print('VIEW %s err=%r' % (name, e), flush=True)

# 画布相关 id 重点列出
for name, cid in [('CV', pp_app.CV), ('MINI', pp_app.MINI), ('TBL', pp_app.TBL),
                  ('ST_PANEL', pp_app.ST_PANEL), ('MI_PANEL', pp_app.MI_PANEL),
                  ('ED_PANEL', pp_app.ED_PANEL), ('SPLIT', pp_app.SPLIT),
                  ('DT_PANEL', pp_app.DT_PANEL)]:
    for r in rows:
        if r[0] == cid:
            print('KEY %-9s id=%d win=%s client=%s' % (name, cid, r[2], r[3]), flush=True)

shot = r'G:\PyArx\docs\_diag_surface_%s.png' % ('on' if mode == '1' else 'off')
app.win.savePng(shot)
print('saved %s' % shot, flush=True)

# 真实屏幕抓图（CopyFromScreen），存 BGRA BMP 供跨进程逐像素 diff
u32.SetWindowPos(hwnd, wt.HWND(-1), 0, 0, 0, 0, 0x0002 | 0x0001 | 0x0010 | 0x0040)
u32.SetWindowPos(hwnd, wt.HWND(0), 0, 0, 0, 0, 0x0002 | 0x0001 | 0x0010 | 0x0040)
u32.SetForegroundWindow(hwnd)
M.pump(150)
grab = Grabber(); sr = grab.rect(hwnd); arr = grab.frame(sr)
tag = 'on' if mode == '1' else 'off'
if arr is not None:
    np.save(r'G:\PyArx\docs\_diag_screen_%s.npy' % tag, arr)
    print('screen npy saved %dx%d' % (arr.shape[1], arr.shape[0]), flush=True)
else:
    print('screen grab failed', flush=True)
