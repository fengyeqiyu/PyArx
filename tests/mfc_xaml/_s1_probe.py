# -*- coding: utf-8 -*-
"""S1 设备层探针：按 PYARX_SURFACE 环境变量跑新表面/旧 GDI 路径，端到端取证。

用法: python _s1_probe.py <mode>
  shot    静态截图 + 客户区网格像素采样
  focus   原生 Edit 焦点 + 键盘输入取证
  drag    分割条拖动：逐帧抓图，连续帧像素差（无旧内容残留）
  pan     画布平移：逐帧抓图，连续帧像素差
  scroll  表格滚动：逐帧抓图，连续帧像素差
  resize  窗口放大/缩小：ResizeBuffers 后画面取证
  lost    强制设备丢失 → 重建 → 画面恢复取证
  diag    只取表面诊断串（探针消息 3 → 调试日志）
"""
import ctypes
import ctypes.wintypes as wt
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
sys.path.insert(0, r'G:\PyArx\pylibs')

import numpy as np
import PyMfcXaml as M
import pp_app

u32 = ctypes.WinDLL('user32', use_last_error=True)
g32 = ctypes.WinDLL('gdi32', use_last_error=True)

PROBE_MSG = 0x521
GW_CHILD, GW_HWNDNEXT, GWL_ID = 5, 2, -12
WM_LBUTTONDOWN, WM_MOUSEMOVE, WM_LBUTTONUP = 0x0201, 0x0200, 0x0202
WM_MOUSEWHEEL = 0x020A
WM_CHAR = 0x0102

u32.GetWindow.argtypes = [wt.HWND, wt.UINT]
u32.GetWindow.restype = wt.HWND
u32.GetWindowLongPtrW.argtypes = [wt.HWND, ctypes.c_int]
u32.GetWindowLongPtrW.restype = ctypes.c_ssize_t
u32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
u32.SendMessageW.restype = ctypes.c_ssize_t
u32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u32.GetFocus.argtypes = []
u32.GetFocus.restype = wt.HWND
u32.SetForegroundWindow.argtypes = [wt.HWND]
u32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                              ctypes.c_int, ctypes.c_int, wt.UINT]
u32.MoveWindow.argtypes = [wt.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                            ctypes.c_int, wt.BOOL]
HWND_TOPMOST = -1
HWND_TOP = 0
SWP_NOACTIVATE = 0x0010
SWP_SHOWWINDOW = 0x0040
SWP_NOMOVE = 0x0002
SWP_NOSIZE = 0x0001


def bring_to_front(hwnd):
    u32.SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW)
    u32.SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW)
    u32.SetForegroundWindow(hwnd)
g32.CreateDIBSection.argtypes = [wt.HDC, ctypes.c_void_p, wt.UINT,
                                 ctypes.POINTER(ctypes.c_void_p), wt.HANDLE, wt.DWORD]
g32.CreateDIBSection.restype = wt.HBITMAP
g32.CreateCompatibleDC.argtypes = [wt.HDC]
g32.CreateCompatibleDC.restype = wt.HDC
g32.SelectObject.argtypes = [wt.HDC, wt.HGDIOBJ]
g32.SelectObject.restype = wt.HGDIOBJ
g32.BitBlt.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                       wt.HDC, ctypes.c_int, ctypes.c_int, wt.DWORD]
g32.BitBlt.restype = wt.BOOL
g32.DeleteObject.argtypes = [wt.HGDIOBJ]
g32.DeleteDC.argtypes = [wt.HDC]


def find_by_id(parent, cid):
    h = u32.GetWindow(parent, GW_CHILD)
    while h:
        if u32.GetWindowLongPtrW(h, GWL_ID) == cid:
            return h
        sub = find_by_id(h, cid)
        if sub:
            return sub
        h = u32.GetWindow(h, GW_HWNDNEXT)
    return None


class _BIH(ctypes.Structure):
    _fields_ = [
        ("biSize", wt.UINT), ("biWidth", ctypes.c_long), ("biHeight", ctypes.c_long),
        ("biPlanes", wt.WORD), ("biBitCount", wt.WORD), ("biCompression", wt.DWORD),
        ("biSizeImage", wt.DWORD), ("biXPelsPerMeter", ctypes.c_long),
        ("biYPelsPerMeter", ctypes.c_long), ("biClrUsed", wt.DWORD),
        ("biClrImportant", wt.DWORD),
    ]


class Grabber:
    def __init__(self):
        self.screen = u32.GetDC(0)

    def rect(self, hwnd):
        r = wt.RECT()
        u32.GetWindowRect(hwnd, ctypes.byref(r))
        return r

    def frame(self, rect):
        w = rect.right - rect.left
        h = rect.bottom - rect.top
        if w <= 0 or h <= 0:
            return None
        bi = _BIH()
        bi.biSize = ctypes.sizeof(_BIH)
        bi.biWidth = w
        bi.biHeight = -h
        bi.biPlanes = 1
        bi.biBitCount = 32
        bi.biCompression = 0
        bits = ctypes.c_void_p()
        dib = g32.CreateDIBSection(self.screen, ctypes.byref(bi), 0,
                                    ctypes.byref(bits), None, 0)
        if dib is None or bits.value is None:
            return None
        mem = g32.CreateCompatibleDC(self.screen)
        old = g32.SelectObject(mem, dib)
        g32.BitBlt(mem, 0, 0, w, h, self.screen, rect.left, rect.top, 0x00CC0020)
        raw = ctypes.string_at(bits, w * h * 4)
        arr = np.frombuffer(raw, dtype=np.uint8).reshape(h, w, 4).copy()
        g32.SelectObject(mem, old)
        g32.DeleteObject(dib)
        g32.DeleteDC(mem)
        return arr

    @staticmethod
    def diff(a, b, thresh=10):
        if a is None or b is None:
            return -1.0, -1
        d = np.abs(a.astype(np.int16) - b.astype(np.int16))
        mask = (d[:, :, 0] > thresh) | (d[:, :, 1] > thresh) | (d[:, :, 2] > thresh)
        return float(mask.mean()), int(mask.sum())


def pump(ms):
    M.pump(ms)


def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else 'shot'
    app = pp_app.App()
    app.win.show()
    pump(80)
    app.showEditor()
    pump(250)

    hwnd = app.win.hwnd
    bring_to_front(hwnd)
    pump(120)

    grab = Grabber()
    rect = grab.rect(hwnd)
    print('HWND=%s rect=(%d,%d,%d,%d) %dx%d'
          % (hwnd, rect.left, rect.top, rect.right, rect.bottom,
             rect.right - rect.left, rect.bottom - rect.top), flush=True)

    # 表面诊断（=1 时写入调试日志；=0 时无表面可探针）
    u32.SendMessageW(hwnd, PROBE_MSG, 3, 0)

    if mode == 'diag':
        return

    if mode == 'shot':
        shot = os.environ.get('S1_SHOT_PNG', r'G:\PyArx\docs\_s1_surface1.png')
        app.win.savePng(shot)
        # 客户区网格采样（窗口边框外的点跳过）
        cr = wt.RECT()
        u32.GetClientRect(hwnd, ctypes.byref(cr))
        pts = []
        for fy in (0.1, 0.3, 0.5, 0.7, 0.9):
            for fx in (0.1, 0.3, 0.5, 0.7, 0.9):
                x = rect.left + int((rect.right - rect.left) * fx)
                y = rect.top + int((rect.bottom - rect.top) * fy)
                px = g32.GetPixel(grab.screen, x, y)
                pts.append((fx, fy, px & 0xFF, (px >> 8) & 0xFF, (px >> 16) & 0xFF))
        for p in pts:
            print('px x=%.1f y=%.1f B=%d G=%d R=%d' % p, flush=True)
        return

    if mode == 'focus':
        edit = find_by_id(hwnd, pp_app.E_SEARCH)
        print('E_SEARCH HWND=%s' % edit, flush=True)
        app.win.setFocus(pp_app.E_SEARCH)
        pump(60)
        focus = u32.GetFocus()
        print('GetFocus=%s expected=%s match=%d' % (focus, edit, focus == edit), flush=True)
        for ch in 'ABC':
            u32.SendMessageW(edit, WM_CHAR, ord(ch), 0)
        pump(60)
        txt = app.win.getText(pp_app.E_SEARCH)
        print('edit text=%r expect contains ABC: %d' % (txt, 'ABC' in txt), flush=True)
        return

    # 交互场景：定位控件、逐帧抓图、连续帧差
    if mode == 'drag':
        cid = pp_app.SPLIT
    elif mode == 'pan':
        cid = pp_app.CV
    elif mode == 'scroll':
        cid = pp_app.TBL
    else:
        cid = 0
    child = find_by_id(hwnd, cid) if cid else hwnd
    cr = grab.rect(child)
    print('child id=%d HWND=%s rect=(%d,%d,%d,%d)'
          % (cid, child, cr.left, cr.top, cr.right, cr.bottom), flush=True)

    frames = []
    if mode == 'drag':
        cw, _ = app.clientSize()
        w0 = app._sideWidth(cw)
        u32.SendMessageW(child, WM_LBUTTONDOWN, 1, 0)
        pump(30)
        for step in range(1, 11):
            x = cw - w0 - pp_app.SPLIT_W - 100 + step * 10
            app.onSplitDrag('move', x)
            pump(12)
            frames.append(grab.frame(rect))
        app.onSplitDrag('up', cw - w0 - pp_app.SPLIT_W - 100 + 90)
        pump(60)
    elif mode == 'winmove':
        x0, y0 = rect.left, rect.top
        w0, h0 = rect.right - rect.left, rect.bottom - rect.top
        for step in range(1, 11):
            u32.MoveWindow(hwnd, x0 + step * 6, y0 + step * 4, w0, h0, True)
            pump(12)
            frames.append(grab.frame(grab.rect(hwnd)))
        pump(60)
    elif mode == 'pan':
        u32.SendMessageW(child, WM_LBUTTONDOWN, 1, (200) | (200 << 16))
        pump(30)
        for step in range(1, 11):
            u32.SendMessageW(child, WM_MOUSEMOVE, 1, (200 + step * 12) | ((200 + step * 8) << 16))
            pump(12)
            frames.append(grab.frame(rect))
        u32.SendMessageW(child, WM_LBUTTONUP, 0, 0)
        pump(60)
    elif mode == 'scroll':
        for step in range(1, 11):
            u32.SendMessageW(child, WM_MOUSEWHEEL, (-120 << 16), (200) | (200 << 16))
            pump(12)
            frames.append(grab.frame(rect))
        pump(60)
    elif mode == 'resize':
        base = (rect.right - rect.left, rect.bottom - rect.top)
        bring_to_front(hwnd)
        u32.SetWindowPos(hwnd, HWND_TOP, rect.left, rect.top, base[0] + 80, base[1] + 60,
                         SWP_NOACTIVATE | SWP_SHOWWINDOW)
        pump(120)
        bring_to_front(hwnd)
        frames.append(grab.frame(grab.rect(hwnd)))
        app.win.savePng(r'G:\PyArx\docs\_s1_resize_big.png')
        u32.SetWindowPos(hwnd, HWND_TOP, rect.left, rect.top, base[0] - 80, base[1] - 60,
                         SWP_NOACTIVATE | SWP_SHOWWINDOW)
        pump(120)
        bring_to_front(hwnd)
        frames.append(grab.frame(grab.rect(hwnd)))
        app.win.savePng(r'G:\PyArx\docs\_s1_resize_small.png')
        r2 = grab.rect(hwnd)
        print('after resize rect=(%d,%d,%d,%d) %dx%d'
              % (r2.left, r2.top, r2.right, r2.bottom, r2.right - r2.left,
                 r2.bottom - r2.top), flush=True)
        return
    elif mode == 'lost':
        # 1 = 模拟设备丢失；下一帧 BeginFrame 触发 Recreate 全量重建
        u32.SendMessageW(hwnd, PROBE_MSG, 1, 0)
        pump(30)
        pump(120)  # 触发 WM_PAINT → BeginFrame → Recreate
        u32.SendMessageW(hwnd, PROBE_MSG, 3, 0)
        frames.append(grab.frame(rect))
        app.win.savePng(r'G:\PyArx\docs\_s1_after_recreate.png')
        return

    ratios = []
    for i in range(1, len(frames)):
        ratio, cnt = grab.diff(frames[i - 1], frames[i])
        ratios.append(ratio)
        print('frame %d->%d changed=%.4f (%d px)' % (i - 1, i, ratio, cnt), flush=True)
    if ratios:
        print('mean frame delta=%.4f max=%.4f'
              % (sum(ratios) / len(ratios), max(ratios)), flush=True)
    app.win.savePng(r'G:\PyArx\docs\_s1_%s.png' % mode)


if __name__ == '__main__':
    main()
