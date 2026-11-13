# -*- coding: utf-8 -*-
"""S1V 六场景 =1/=0 同状态抓帧探针。

用法:
    $env:PYARX_SURFACE='0'; python _s1v_scene.py
    $env:PYARX_SURFACE='1'; python _s1v_scene.py

每场景抓一张整窗屏幕 DIB，存 docs\\_s1v_<surface>_<scene>.npy，
由 _s1v_compare.py 逐场景做像素对照。只取证，不进生产链路。
"""
import ctypes
import ctypes.wintypes as wt
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
sys.path.insert(0, r'G:\PyArx\pylibs')

import numpy as np
import PyMfcXaml as M
import pp_app
import pp_events as E

u32 = ctypes.WinDLL('user32', use_last_error=True)
g32 = ctypes.WinDLL('gdi32', use_last_error=True)

GW_CHILD, GW_HWNDNEXT, GWL_ID = 5, 2, -12
WM_LBUTTONDOWN, WM_MOUSEMOVE, WM_LBUTTONUP = 0x0201, 0x0200, 0x0202
WM_MOUSEWHEEL = 0x020A
MK_LBUTTON = 0x0001

u32.GetWindow.argtypes = [wt.HWND, wt.UINT]
u32.GetWindow.restype = wt.HWND
u32.GetWindowLongPtrW.argtypes = [wt.HWND, ctypes.c_int]
u32.GetWindowLongPtrW.restype = ctypes.c_ssize_t
u32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
u32.SendMessageW.restype = ctypes.c_ssize_t
u32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                             ctypes.c_int, ctypes.c_int, wt.UINT]
u32.MoveWindow.argtypes = [wt.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                           ctypes.c_int, wt.BOOL]

SURF = os.environ.get('PYARX_SURFACE', '0')
OUT = r'G:\PyArx\docs'


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


def grab_frame(rect):
    w = rect.right - rect.left
    h = rect.bottom - rect.top
    bi = _BIH()
    bi.biSize = ctypes.sizeof(_BIH)
    bi.biWidth = w
    bi.biHeight = -h
    bi.biPlanes = 1
    bi.biBitCount = 32
    bi.biCompression = 0
    bits = ctypes.c_void_p()
    screen = u32.GetDC(0)
    dib = g32.CreateDIBSection(screen, ctypes.byref(bi), 0,
                              ctypes.byref(bits), None, 0)
    mem = g32.CreateCompatibleDC(screen)
    old = g32.SelectObject(mem, dib)
    g32.BitBlt(mem, 0, 0, w, h, screen, rect.left, rect.top, 0x00CC0020)
    raw = ctypes.string_at(bits, w * h * 4)
    arr = np.frombuffer(raw, dtype=np.uint8).reshape(h, w, 4).copy()
    g32.SelectObject(mem, old)
    g32.DeleteObject(dib)
    g32.DeleteDC(mem)
    u32.ReleaseDC(0, screen)
    return arr


def save(scene, arr):
    path = os.path.join(OUT, '_s1v_%s_%s.npy' % (SURF, scene))
    np.save(path, arr)
    print('saved %s shape=%s' % (os.path.basename(path), arr.shape), flush=True)


def main():
    app = pp_app.App()
    app.win.show()
    M.pump(120)
    app.showEditor()
    M.pump(300)

    hwnd = app.win.hwnd
    u32.SetWindowPos(hwnd, 0, 100, 80, 0, 0, 0x0003)  # NOMOVE NOSIZE
    u32.SetWindowPos(hwnd, -1, 100, 80, 0, 0, 0x0003)
    u32.SetWindowPos(hwnd, -2, 100, 80, 0, 0, 0x0003)
    u32.SetForegroundWindow(hwnd)
    M.pump(200)

    rect = wt.RECT()
    u32.GetWindowRect(hwnd, ctypes.byref(rect))
    print('HWND=%s rect=(%d,%d,%d,%d) %dx%d surface=%s'
          % (hwnd, rect.left, rect.top, rect.right, rect.bottom,
             rect.right - rect.left, rect.bottom - rect.top, SURF), flush=True)

    # 1. 初始
    save('initial', grab_frame(rect))

    # 2. 拖窗口（移动固定 12,8，尺寸不变）
    u32.MoveWindow(hwnd, rect.left + 12, rect.top + 8,
                   rect.right - rect.left, rect.bottom - rect.top, True)
    M.pump(120)
    rw = wt.RECT()
    u32.GetWindowRect(hwnd, ctypes.byref(rw))
    save('winmove', grab_frame(rw))

    # 3. 拖分割条一步
    spl = find_by_id(hwnd, pp_app.SPLIT)
    cw, _ = app.clientSize()
    w0 = app._sideWidth(cw)
    u32.SendMessageW(spl, WM_LBUTTONDOWN, MK_LBUTTON, 0)
    M.pump(30)
    app.onSplitDrag('move', cw - w0 - pp_app.SPLIT_W + 40)
    M.pump(120)
    u32.SendMessageW(spl, WM_LBUTTONUP, 0, 0)
    M.pump(60)
    save('split', grab_frame(rw))

    # 4. 画布平移
    cv = find_by_id(hwnd, pp_app.CV)
    u32.SendMessageW(cv, WM_LBUTTONDOWN, MK_LBUTTON, (200) | (200 << 16))
    M.pump(30)
    u32.SendMessageW(cv, WM_MOUSEMOVE, MK_LBUTTON, (230) | ((218) << 16))
    M.pump(120)
    u32.SendMessageW(cv, WM_LBUTTONUP, 0, 0)
    M.pump(60)
    save('pan', grab_frame(rw))

    # 5. 表格滚动（两格滚轮）
    tbl = find_by_id(hwnd, pp_app.TBL)
    for _ in range(2):
        u32.SendMessageW(tbl, WM_MOUSEWHEEL, (-120 << 16), (200) | (200 << 16))
        M.pump(60)
    save('scroll', grab_frame(rw))

    # 6. 弹详情：真实消息点中第二张卡片
    code = app.nodes[2]['code']
    rc = app.win.cardRectPlan(pp_app.CV, code)
    cx, cy = int(rc[0] + rc[2] / 2), int(rc[1] + rc[3] / 2)
    E.click(cv, cx, cy, pump=120)
    M.pump(120)
    print('detail visible=%s code=%s' % (app._detailVisible, code), flush=True)
    save('detail', grab_frame(rw))

    print('=== scene capture done surface=%s ===' % SURF, flush=True)
    os._exit(0)


if __name__ == '__main__':
    main()
