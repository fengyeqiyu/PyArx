# -*- coding: utf-8 -*-
"""悬浮滚轮探针：鼠标（坐标）压在滚动条上但焦点在别处时，滚轮能不能滚。

做法：用 PostMessage 投递 WM_MOUSEWHEEL，lParam 里放"屏幕坐标"，不打桩真实鼠标；
宿主的 WH_GETMESSAGE 钩子按该坐标找光标下的控件并分流。
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
user32.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
user32.PostMessageW.restype = wintypes.BOOL
user32.GetDlgCtrlID.argtypes = [wintypes.HWND]
user32.GetDlgCtrlID.restype = ctypes.c_int
user32.GetWindowRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
user32.GetWindowRect.restype = wintypes.BOOL
user32.EnumChildWindows.argtypes = [wintypes.HWND, ctypes.c_void_p, wintypes.LPARAM]
user32.ScreenToClient.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.POINT)]
user32.SetFocus.argtypes = [wintypes.HWND]
user32.SetFocus.restype = wintypes.HWND
user32.SetWindowPos.argtypes = [wintypes.HWND, wintypes.HWND, ctypes.c_int, ctypes.c_int,
                                ctypes.c_int, ctypes.c_int, wintypes.UINT]
user32.SetWindowPos.restype = wintypes.BOOL
user32.SetForegroundWindow.argtypes = [wintypes.HWND]
user32.SetForegroundWindow.restype = wintypes.BOOL

HWND_TOPMOST = wintypes.HWND(-1)
SWP_NOSIZE = 0x0001
SWP_NOMOVE = 0x0002
SWP_SHOWWINDOW = 0x0040

WM_MOUSEWHEEL = 0x020A
# delta 在高字：0xFF88 = -120（滚轮向下），0x0078 = +120（滚轮向上）
DOWN = 0xFF880000
UP = 0x00780000

HITS = {}


def onValue(*a):
    HITS['value'] = HITS.get('value', 0) + 1


def onScroll(*a):
    HITS['scroll'] = HITS.get('scroll', 0) + 1


XAML = '''<Window Title="悬浮滚轮探针" Width="640" Height="420">
  <TextBox Name="T" Left="16" Top="16" Width="240" Height="28" Text="焦点在这里"/>
  <ScrollBar Name="SB" Left="16" Top="80" Width="260" Height="18"
             Minimum="0" Maximum="100" PageSize="10" Value="0" ValueChanged="onValue"/>
  <ScrolledWindow Name="SW" Left="16" Top="140" Width="260" Height="200" Scroll="onScroll"/>
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


def rect(hwnd):
    r = wintypes.RECT()
    user32.GetWindowRect(hwnd, ctypes.byref(r))
    return r


def xy(x, y):
    return (y << 16) | (x & 0xFFFF)


def wheel(target, x, y, wp):
    user32.PostMessageW(target, WM_MOUSEWHEEL, wp, xy(x, y))


def who(x, y):
    """诊断：这个屏幕坐标上 WindowFromPoint 命中谁。"""
    pt = wintypes.POINT(x, y)
    h = user32.WindowFromPoint(pt)
    cls = ctypes.create_unicode_buffer(128)
    if h:
        user32.GetClassNameW(h, cls, 128)
    return h, cls.value, user32.GetDlgCtrlID(h) if h else 0


def main():
    path = r'G:\PyArx\docs\_sim_wheel.xaml'
    with open(path, 'w', encoding='utf-8') as f:
        f.write(XAML)

    names = {}
    win = M.guiload(path, context=globals(), names=names)
    win.show()
    M.pump(150)

    hw = win.hwnd() if callable(win.hwnd) else win.hwnd
    # 探针依赖 WindowFromPoint 命中我们自己：把窗口顶到最前，避免被终端压住
    user32.SetWindowPos(hw, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW)
    user32.SetForegroundWindow(hw)
    M.pump(120)
    kids = children(hw)
    sbid, swid, tid = names['SB'], names['SW'], names['T']
    sb, sw, tb = kids[sbid], kids[swid], kids[tid]
    print('SB=%r SW=%r T=%r' % (sb, sw, tb))

    r = rect(sb)
    sbx, sby = (r.left + r.right) // 2, (r.top + r.bottom) // 2
    rs = rect(sw)
    swx, swy = rs.right - 8, (rs.top + rs.bottom) // 2      # 右侧条带
    rt = rect(tb)
    tx, ty = (rt.left + rt.right) // 2, (rt.top + rt.bottom) // 2

    # 焦点先给别的控件（TextBox），证明滚动条没焦点也能滚
    user32.SetFocus(tb)
    M.pump(40)
    print('focus=%r' % user32.GetFocus())

    win.setScrollRange(swid, 0, 600)
    M.pump(40)
    print('命中诊断：SB点=%r  SW条带=%r  TextBox点=%r' % (who(sbx, sby), who(swx, swy), who(tx, ty)))
    base = win.getScrollBarValue(sbid)
    sp0 = win.getScrollPos(swid)

    # 1) 光标压在独立 ScrollBar 上，滚轮下滚（消息投给框架窗口，焦点在 TextBox）
    wheel(hw, sbx, sby, DOWN)
    M.pump(80)
    v1 = win.getScrollBarValue(sbid)
    print('  [1] wheel之后 value=%d' % v1)
    check(v1 > base, '悬浮在 ScrollBar 上，滚轮下滚生效（焦点在 TextBox）', v1)
    check(HITS.get('value', 0) >= 1, '悬浮滚轮也发 ValueChanged', HITS.get('value', 0))

    # 2) 上滚
    wheel(hw, sbx, sby, UP)
    M.pump(80)
    v2 = win.getScrollBarValue(sbid)
    print('  [2] wheel之后 value=%d' % v2)
    check(v2 < v1, '悬浮在 ScrollBar 上，滚轮上滚生效', v2)

    # 3) 光标压在 ScrolledWindow 右侧滚动条条带上
    wheel(hw, swx, swy, DOWN)
    M.pump(80)
    sp1 = win.getScrollPos(swid)
    print('  [3] wheel之后 pos=%d' % sp1)
    check(sp1 > sp0, '悬浮在滚动区域条带上，滚轮生效', sp1)
    check(HITS.get('scroll', 0) >= 1, '滚动区域的 Scroll 事件回调', HITS.get('scroll', 0))

    # 4) 光标压在 TextBox 上（不在任何滚动条上）→ 两者都不该动
    v3 = win.getScrollBarValue(sbid)
    sp2 = win.getScrollPos(swid)
    wheel(hw, tx, ty, DOWN)
    M.pump(80)
    check(win.getScrollBarValue(sbid) == v3, '光标在普通控件上时不影响 ScrollBar',
          win.getScrollBarValue(sbid))
    check(win.getScrollPos(swid) == sp2, '光标在普通控件上时不影响滚动区域',
          win.getScrollPos(swid))

    print('\nHITS=%r  SB=%d  SW=%d' % (HITS, win.getScrollBarValue(sbid), win.getScrollPos(swid)))
    print('结果：%d 项失败' % len(fails))
    win.close()
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
