# -*- coding: utf-8 -*-
"""_probe_edit_focus.py — 输入框自绘取证：焦点边框 + 选区 + 文字都出自皮肤层。
用 EM_SETSEL 选中全文，截图里应看到强调色底 + 白字（原生 EDIT 也这样，但边框不会变蓝）。
"""
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
    target = edits[2] if edits else None
    if target:
        r = wt.RECT()
        u.GetWindowRect(target, ctypes.byref(r))
        cx, cy = (r.left + r.right) // 2, (r.top + r.bottom) // 2
        u.SetCursorPos(cx, cy)
        time.sleep(0.1)
        u.mouse_event(0x0002, 0, 0, 0, 0)
        time.sleep(0.05)
        u.mouse_event(0x0004, 0, 0, 0, 0)
        time.sleep(0.3)
        u.SendMessageW(target, 0x00B1, 0, 0xFFFFFFFF)   # EM_SETSEL(0, -1) 全选
        time.sleep(0.4)
        print('sel =', u.SendMessageW(target, 0x00B0, 0, 0))   # EM_GETSEL（低字起、高字止）

    r = wt.RECT()
    u.GetWindowRect(h, ctypes.byref(r))
    base.grab(r.left, r.top, r.right - r.left, r.bottom - r.top, os.path.join(OUT, '_vo_edit.bmp'))
    print('saved _vo_edit.bmp')
    p.terminate()
