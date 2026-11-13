# -*- coding: utf-8 -*-
"""_probe_edit_settext.py — 只改文字（WM_SETTEXT），确认预览器输入框的重画是否只由皮肤层出像素。
若截图里同一段文字出现两行（一上一下），说明原生 EDIT 仍在画自己那份。"""
import ctypes
import ctypes.wintypes as wt
import os
import subprocess
import time

u = ctypes.windll.user32
u.SetProcessDPIAware()
u.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wt.LPARAM]
u.GetClassNameW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
u.SendMessageW.argtypes = [wt.HWND, ctypes.c_uint, ctypes.c_size_t, ctypes.c_size_t]
u.SendMessageW.restype = ctypes.c_size_t

import _probe_viewer_shot as base

VIEWER = r'G:\PyArx\X64\Release\Viewer.exe'
XAML = r'G:\PyArx\tests\mfc_controls\level_tab_viewer.xaml'
OUT = r'G:\PyArx\docs'


def children(h):
    out = []

    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(c, _):
        out.append(c)
        return 1

    u.EnumChildWindows(ctypes.cast(h, ctypes.c_void_p), ctypes.cast(cb, ctypes.c_void_p), 0)
    return out


def cls_of(h):
    b = ctypes.create_unicode_buffer(128)
    u.GetClassNameW(h, b, 128)
    return b.value


if __name__ == '__main__':
    p = subprocess.Popen([VIEWER, XAML], cwd=os.path.dirname(VIEWER))
    h = None
    for _ in range(30):
        time.sleep(0.5)
        h = base.find('预览器 · 层次与 Tab 序号')
        if h:
            break
    if not h:
        print('未找到预览器窗口')
        p.terminate()
        raise SystemExit(1)
    u.SetForegroundWindow(h)
    time.sleep(1.0)

    edits = [c for c in children(h) if cls_of(c).upper() in ('EDIT', 'RICHEDIT50W', 'RICHEDIT20W')]
    target = edits[2] if edits else None   # 声明顺序第一只
    if target:
        buf = ctypes.create_unicode_buffer('HELLO 自绘')   # 必须留住引用，临时对象会被回收
        u.SendMessageW(target, 0x000C, 0, ctypes.addressof(buf))  # WM_SETTEXT
        time.sleep(0.4)
        print('cls=', cls_of(target))

    r = wt.RECT()
    u.GetWindowRect(h, ctypes.byref(r))
    base.grab(r.left, r.top, r.right - r.left, r.bottom - r.top, os.path.join(OUT, '_vo_settext.bmp'))
    print('saved _vo_settext.bmp')
    p.terminate()
