# -*- coding: utf-8 -*-
"""ScrollBar 交互探针：模拟消息点轨道 / 拖滑块 / 滚轮，看值变化与 ValueChanged 回调。

结论用数据说话：
  * 值变了 -> 控件内部状态有效
  * HITS 里有没有 'value' -> 是否通知到 Python
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
user32.GetDlgCtrlID.argtypes = [wintypes.HWND]
user32.GetDlgCtrlID.restype = ctypes.c_int
user32.EnumChildWindows.argtypes = [wintypes.HWND, ctypes.c_void_p, wintypes.LPARAM]

WM_LBUTTONDOWN = 0x0201
WM_LBUTTONUP = 0x0202
WM_MOUSEMOVE = 0x0200
WM_MOUSEWHEEL = 0x020A
MK_LBUTTON = 0x0001

HITS = {}


def onValue(*a):
    HITS['value'] = HITS.get('value', 0) + 1


XAML = '''<Window Title="滚动条探针" Width="520" Height="300">
  <ScrollBar Name="SB" Left="16" Top="40" Width="260" Height="18"
             Minimum="0" Maximum="100" PageSize="10" Value="0" ValueChanged="onValue"/>
</Window>'''

fails = []


def check(cond, what, got=None):
    print(('OK   ' if cond else 'FAIL ') + what + ('  got=%r' % (got,) if got is not None else ''))
    if not cond:
        fails.append(what)


def children(root):
    found = {}
    proto = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

    def cb(hwnd, _lparam):
        found[user32.GetDlgCtrlID(hwnd)] = hwnd
        return True

    user32.EnumChildWindows(root, proto(cb), 0)
    return found


def lparam(x, y):
    return (y << 16) | (x & 0xFFFF)


def main():
    path = r'G:\PyArx\docs\_sim_scrollbar.xaml'
    with open(path, 'w', encoding='utf-8') as f:
        f.write(XAML)

    names = {}
    win = M.guiload(path, context=globals(), names=names)
    win.show()
    M.pump(120)

    hw = win.hwnd() if callable(win.hwnd) else win.hwnd
    kids = children(hw)
    sid = names['SB']
    sh = kids.get(sid)
    print('ScrollBar id=%d hwnd=%r' % (sid, sh))
    if not sh:
        print('找不到滚动条窗口，退出')
        return 1

    v0 = win.getScrollBarValue(sid)
    print('初始 Value=%d  HITS=%r' % (v0, HITS))

    # 1) 点轨道右半（滑块在最左，宽度 260 => 点 x=220 必定在轨道上）
    user32.SendMessageW(sh, WM_LBUTTONDOWN, MK_LBUTTON, lparam(220, 9))
    user32.SendMessageW(sh, WM_LBUTTONUP, 0, lparam(220, 9))
    v1 = win.getScrollBarValue(sid)
    check(v1 == v0 + 10, '点轨道跳一页（0 -> 10）', v1)
    check(HITS.get('value', 0) >= 1, '点轨道后 ValueChanged 通知到 Python', HITS.get('value', 0))

    # 2) 再点轨道左半，应回到 0
    user32.SendMessageW(sh, WM_LBUTTONDOWN, MK_LBUTTON, lparam(5, 9))
    user32.SendMessageW(sh, WM_LBUTTONUP, 0, lparam(5, 9))
    v2 = win.getScrollBarValue(sid)
    check(v2 == v1 - 10, '点另一侧回退一页', v2)

    # 3) 拖滑块：在滑块上按下，往右拖
    user32.SendMessageW(sh, WM_LBUTTONDOWN, MK_LBUTTON, lparam(6, 9))
    user32.SendMessageW(sh, WM_MOUSEMOVE, MK_LBUTTON, lparam(160, 9))
    user32.SendMessageW(sh, WM_LBUTTONUP, 0, lparam(160, 9))
    v3 = win.getScrollBarValue(sid)
    check(v3 > v2, '拖滑块改变值', v3)
    check(HITS.get('value', 0) >= 2, '拖滑块也通知 ValueChanged', HITS.get('value', 0))

    # 4) 滚轮：半页
    user32.SendMessageW(sh, WM_MOUSEWHEEL, 0xFF880000, lparam(100, 9))   # delta = -120
    v4 = win.getScrollBarValue(sid)
    print('滚轮后 Value=%d  总回调=%r' % (v4, HITS))
    check(v4 != v3, '滚轮改变值', v4)

    print('\nHITS=%r' % HITS)
    print('结果：%d 项失败' % len(fails))
    win.close()
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
