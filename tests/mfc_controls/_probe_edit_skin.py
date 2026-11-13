# -*- coding: utf-8 -*-
"""_probe_edit_skin.py — 输入框自绘取证：点进 e1、打字、截图。
判据：框内文字随后输入更新（说明像素由皮肤层画、文字取自控件状态），
      边框在获得焦点时变成强调色（原生 EDIT 不会这样画）。
"""
import ctypes
import ctypes.wintypes as wt
import os
import subprocess
import time

u = ctypes.windll.user32
u.SetProcessDPIAware()
u.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u.EnumWindows.argtypes = [ctypes.c_void_p, wt.LPARAM]
u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wt.LPARAM]
u.GetWindowTextW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
u.SendMessageW.argtypes = [wt.HWND, ctypes.c_uint, ctypes.c_size_t, ctypes.c_size_t]
u.SendMessageW.restype = ctypes.c_size_t

VIEWER = r'G:\PyArx\X64\Release\Viewer.exe'
XAML = r'G:\PyArx\tests\mfc_controls\level_tab_viewer.xaml'
OUT = r'G:\PyArx\docs'

import _probe_viewer_shot as base  # grab / find 复用


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


def text_of(h):
    b = ctypes.create_unicode_buffer(512)
    u.SendMessageW(h, 0x000D, 512, ctypes.addressof(b))   # WM_GETTEXT
    return b.value


def click(x, y):
    u.SetCursorPos(x, y)
    time.sleep(0.1)
    u.mouse_event(0x0002, 0, 0, 0, 0)
    time.sleep(0.05)
    u.mouse_event(0x0004, 0, 0, 0, 0)
    time.sleep(0.3)


def key(char):
    vk = 0x41 if char == 'A' else 0
    u.keybd_event(vk, 0, 0, 0)
    time.sleep(0.03)
    u.keybd_event(vk, 0, 2, 0)
    time.sleep(0.2)


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
    print('输入框子窗口：', len(edits))
    for c in edits:
        r = wt.RECT()
        u.GetWindowRect(c, ctypes.byref(r))
        print('  hwnd=%d cls=%s text=%r rect=%s' % (c, cls_of(c), text_of(c),
                                                   (r.left, r.top, r.right, r.bottom)))

    r = wt.RECT()
    u.GetWindowRect(h, ctypes.byref(r))
    print('窗口 rect=', (r.left, r.top, r.right, r.bottom))

    # 点进第一个输入框、打一个 A
    if edits:
        er = wt.RECT()
        u.GetWindowRect(edits[0], ctypes.byref(er))
        cx = (er.left + er.right) // 2
        cy = (er.top + er.bottom) // 2
        click(cx, cy)
        time.sleep(0.2)
        # 直接投 WM_CHAR，绕开中文输入法（IME 合成窗会盖在光标上，干扰取证）
        u.SendMessageW(edits[0], 0x0102, ord('Z'), 0x001C0001)   # WM_CHAR
        time.sleep(0.4)
        print('   输入后 text=%r' % text_of(edits[0]))

    base.grab(r.left, r.top, r.right - r.left, r.bottom - r.top, os.path.join(OUT, '_vo_edit.bmp'))
    print('saved _vo_edit.bmp')
    p.terminate()
