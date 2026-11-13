# -*- coding: utf-8 -*-
"""悬浮光标探针：可点控件手型、画布爪型、容器/空白箭头。

做法：向各控件直接 SendMessage(WM_SETCURSOR, hwnd, MAKELONG(HTCLIENT, WM_MOUSEMOVE))，
再读 GetCursor() 的实际句柄对比。不动真实鼠标。
"""
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

WM_MOUSEMOVE_ = 0x0200
WM_SETCURSOR = 0x0020
WM_MOUSELEAVE = 0x02A3
HTCLIENT = 1

IDC_ARROW = 32512
IDC_HAND = 32649

HAND = user32.LoadCursorW(None, IDC_HAND)
ARROW = user32.LoadCursorW(None, IDC_ARROW)

XAML = '''<Window Title="光标探针" Width="640" Height="460">
  <Button Name="B" Left="16" Top="16" Width="120" Height="32" Text="按钮"/>
  <ToggleButton Name="TG" Left="150" Top="16" Width="120" Height="32" Text="开关按钮"/>
  <hyperlink Name="L" Left="290" Top="20" Width="120" Height="24" Text="链接"/>
  <ScrollBar Name="SB" Left="16" Top="70" Width="260" Height="18"
             Minimum="0" Maximum="100" PageSize="10" Value="0"/>
  <QCanvas Name="CV" Left="16" Top="110" Width="260" Height="150"/>
  <label Name="LB" Left="16" Top="280" Width="160" Height="24" Text="普通文字"/>
  <panel Name="PN" Left="320" Top="60" Width="200" Height="200"/>
</Window>'''

fails = []


def check(cond, what, got=None):
    print(('OK   ' if cond else 'FAIL ') + what + ('  got=%r' % (got,) if got is not None else ''))
    if not cond:
        fails.append(what)


def children(root):
    found = {}
    proto = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

    def cb(hwnd, _l):
        found[user32.GetDlgCtrlID(hwnd)] = hwnd
        return True

    user32.EnumChildWindows(root, proto(cb), 0)
    return found


def cursor_of(hwnd):
    user32.SendMessageW(hwnd, WM_SETCURSOR, hwnd, (HTCLIENT & 0xFFFF) | (WM_MOUSEMOVE_ << 16))
    return user32.GetCursor()


def main():
    path = r'G:\PyArx\docs\_sim_cursor.xaml'
    with open(path, 'w', encoding='utf-8') as f:
        f.write(XAML)

    names = {}
    win = M.guiload(path, context=globals(), names=names)
    win.show()
    M.pump(140)
    hw = win.hwnd() if callable(win.hwnd) else win.hwnd
    kids = children(hw)

    cases = [
        ('B', '按钮', HAND),
        ('TG', 'Toggle 按钮', HAND),
        ('L', '超链接', HAND),
        ('SB', '独立 ScrollBar', HAND),
        ('LB', '普通文字 label', ARROW),
    ]
    for name, what, expect in cases:
        h = kids.get(names[name])
        if not h:
            check(False, '%s 窗口没找到' % what)
            continue
        cur = cursor_of(h)
        check(cur == expect, '%s 光标%s' % (what, '手型' if expect == HAND else '箭头'), cur)

    # 画布：既不是系统手型也不是箭头，而是自绘的爪型
    cv = kids.get(names['CV'])
    cur_cv = cursor_of(cv)
    check(cur_cv not in (HAND, ARROW, None), 'QCanvas 是自绘爪型（非系统手型/箭头）', hex(cur_cv or 0))

    # 面板空白：箭头（容器不该整片变手型）
    pn = kids.get(names['PN'])
    if pn:
        check(cursor_of(pn) == ARROW, 'Panel 空白处是箭头', hex(cursor_of(pn) or 0))

    # 离开控件后复位箭头
    user32.SendMessageW(kids[names['SB']], WM_MOUSELEAVE, 0, 0)
    check(user32.GetCursor() == ARROW, '鼠标离开后复位箭头', hex(user32.GetCursor() or 0))

    print('\n结果：%d 项失败' % len(fails))
    win.close()
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
