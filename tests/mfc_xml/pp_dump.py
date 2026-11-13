# -*- coding: utf-8 -*-
"""打印 ppApp 窗口所有子控件的真实矩形，定位版式漂移。"""
import ctypes
import os
import sys
import time
from ctypes import wintypes

sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M
import pp_app

user32 = ctypes.windll.user32
EnumWindowsProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
EnumChildProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


def main_window():
    hits = []

    def cb(h, l):
        buf = ctypes.create_unicode_buffer(512)
        user32.GetWindowTextW(h, buf, 512)
        if '配电规划图' in buf.value:
            hits.append(h)
        return True

    user32.EnumWindows(EnumWindowsProc(cb), 0)
    return hits[0] if hits else None


def dump(hwnd):
    rows = []

    def cb(h, l):
        pid = wintypes.DWORD(0)
        user32.GetWindowThreadProcessId(h, ctypes.byref(pid))
        if pid.value != os.getpid():
            return True
        cls = ctypes.create_unicode_buffer(128)
        user32.GetClassNameW(h, cls, 128)
        r = wintypes.RECT()
        user32.GetWindowRect(h, ctypes.byref(r))
        vis = bool(user32.IsWindowVisible(h))
        ctl = user32.GetDlgCtrlID(h)
        rows.append((ctl, cls.value, r.left, r.top, r.right - r.left, r.bottom - r.top, vis))
        return True

    user32.EnumChildWindows(hwnd, EnumChildProc(cb), 0)
    return rows


def main():
    app = pp_app.App()
    app.win.show()
    time.sleep(0.6)
    M.pump(40)
    h = main_window()
    if not h:
        print('找不到窗口')
        return
    pr = wintypes.RECT()
    user32.GetWindowRect(h, ctypes.byref(pr))
    print('主窗口 %d,%d %dx%d' % (pr.left, pr.top, pr.right - pr.left, pr.bottom - pr.top))
    print('%-6s %-16s %6s %6s %6s %6s %s' % ('id', 'class', 'x', 'y', 'w', 'h', 'vis'))
    for r in sorted(dump(h), key=lambda x: (x[2], x[3])):
        print('%-6d %-16s %6d %6d %6d %6d %s' % (r[0], r[1][:16], r[2] - pr.left, r[3] - pr.top,
                                                 r[4], r[5], r[6]))


if __name__ == '__main__':
    main()
