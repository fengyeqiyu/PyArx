# -*- coding: utf-8 -*-
"""ScrolledWindow 滚动区域探针：点轨道翻一页、拖拇指、滚轮，并核对 Scroll 事件是否回调。

统计口径：
  * getScrollPos(id) —— 位置真的变了
  * HITS['scroll']  —— Scroll 事件到了 Python
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


def onScroll(*a):
    HITS['scroll'] = HITS.get('scroll', 0) + 1


XAML = '''<Window Title="滚动区探针" Width="560" Height="360">
  <ScrolledWindow Name="SW" Left="16" Top="16" Width="260" Height="200" Scroll="onScroll"/>
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


def lp(x, y):
    return (y << 16) | (x & 0xFFFF)


def main():
    path = r'G:\PyArx\docs\_sim_scrolled.xaml'
    with open(path, 'w', encoding='utf-8') as f:
        f.write(XAML)

    names = {}
    win = M.guiload(path, context=globals(), names=names)
    win.show()
    M.pump(150)

    hw = win.hwnd() if callable(win.hwnd) else win.hwnd
    kids = children(hw)
    sid = names['SW']
    sw = kids.get(sid)
    print('ScrolledWindow id=%d hwnd=%r' % (sid, sw))
    if not sw:
        print('找不到滚动区域窗口')
        return 1

    # 量程 0..600，视口 200 高 => 一页应是 200
    win.setScrollRange(sid, 0, 600)
    M.pump(60)
    p0 = win.getScrollPos(sid)
    print('初始 pos=%d  HITS=%r' % (p0, HITS))
    check(p0 == 0, '初始位置 = 0')

    # 1) 点右侧轨道下半（拇指在顶部）=> 翻一页
    user32.SendMessageW(sw, WM_LBUTTONDOWN, MK_LBUTTON, lp(250, 150))
    user32.SendMessageW(sw, WM_LBUTTONUP, 0, lp(250, 150))
    p1 = win.getScrollPos(sid)
    check(p1 == 200, '点轨道 = 翻一页（0 -> 200，不是一行）', p1)

    # 2) 再点一次 => 再翻一页
    user32.SendMessageW(sw, WM_LBUTTONDOWN, MK_LBUTTON, lp(250, 190))
    user32.SendMessageW(sw, WM_LBUTTONUP, 0, lp(250, 190))
    p2 = win.getScrollPos(sid)
    check(p2 == 400, '再点一次继续翻页', p2)

    # 3) 点轨道上半 => 往回翻一页
    user32.SendMessageW(sw, WM_LBUTTONDOWN, MK_LBUTTON, lp(250, 5))
    user32.SendMessageW(sw, WM_LBUTTONUP, 0, lp(250, 5))
    p3 = win.getScrollPos(sid)
    check(p3 == 200, '点上半区往回翻一页', p3)

    # 4) 拖拇指：拇指在中间，按住往下拖
    thumbY = win.getScrollPos(sid)
    print('拖之前 pos=%d' % thumbY)
    user32.SendMessageW(sw, WM_LBUTTONDOWN, MK_LBUTTON, lp(250, 90))
    user32.SendMessageW(sw, WM_MOUSEMOVE, MK_LBUTTON, lp(250, 170))
    user32.SendMessageW(sw, WM_LBUTTONUP, 0, lp(250, 170))
    p4 = win.getScrollPos(sid)
    check(p4 > p3, '拖拇指改变位置', p4)

    # 5) 滚轮：一档
    user32.SendMessageW(sw, WM_MOUSEWHEEL, 0xFF880000, lp(100, 100))   # delta=-120
    p5 = win.getScrollPos(sid)
    check(p5 > p4, '滚轮改变位置', p5)

    total = HITS.get('scroll', 0)
    check(total >= 4, 'Scroll 事件回调到 Python（点轨道/拖动/滚轮）', total)
    print('\nHITS=%r  最终 pos=%d' % (HITS, p5))
    print('结果：%d 项失败' % len(fails))
    win.close()
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
