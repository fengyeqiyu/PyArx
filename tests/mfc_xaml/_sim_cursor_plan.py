# -*- coding: utf-8 -*-
"""配电画布（PowerCanvas）光标探针：应显示自绘爪型。"""
import ctypes
import os
import sys
from ctypes import wintypes

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
os.environ.setdefault('PYARX_D2D_RT', 'software')

import PyMfcXaml as M

user32 = ctypes.WinDLL('user32', use_last_error=True)
user32.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
user32.SendMessageW.restype = ctypes.c_ssize_t
user32.LoadCursorW.restype = ctypes.c_void_p
user32.GetCursor.restype = ctypes.c_void_p
user32.GetDlgCtrlID.argtypes = [wintypes.HWND]
user32.GetDlgCtrlID.restype = ctypes.c_int
user32.EnumChildWindows.argtypes = [wintypes.HWND, ctypes.c_void_p, wintypes.LPARAM]

HAND = user32.LoadCursorW(None, 32649)
ARROW = user32.LoadCursorW(None, 32512)

XAML = '''<Window Title="配电画布光标" Width="620" Height="440">
  <PowerCanvas Name="PC" Left="12" Top="12" Width="420" Height="300"/>
</Window>'''


def kids(root):
    found = {}
    proto = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

    def cb(h, _l):
        found[user32.GetDlgCtrlID(h)] = h
        return True
    user32.EnumChildWindows(root, proto(cb), 0)
    return found


def main():
    p = r'G:\PyArx\docs\_sim_cursor_plan.xaml'
    with open(p, 'w', encoding='utf-8') as f:
        f.write(XAML)
    names = {}
    win = M.guiload(p, context=globals(), names=names)
    win.show()
    M.pump(150)
    hw = win.hwnd() if callable(win.hwnd) else win.hwnd
    pc = kids(hw).get(names['PC'])
    if not pc:
        print('PowerCanvas 窗口没找到')
        return 1
    user32.SendMessageW(pc, 0x0020, pc, 1 | (0x0200 << 16))    # WM_SETCURSOR / HTCLIENT
    cur = user32.GetCursor()
    ok = cur not in (HAND, ARROW, None)
    print(('OK   ' if ok else 'FAIL ') + 'PowerCanvas 是自绘爪型  got=%s' % hex(cur or 0))
    win.close()
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
