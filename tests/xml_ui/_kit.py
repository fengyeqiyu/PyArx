# -*- coding: utf-8 -*-
"""新测试套件的公共件：加载 PyMfcXml、断言、模拟点击。

和旧套件（tests/mfc_xml/pp_*.py）的区别：
* 失败时进程退出码非 0（旧套件末尾 os._exit(0)，退出码永远是 0，只能靠数日志）；
* 不再依赖 docs/ 下的 ppt/项目 XML，测试自己造窗口和 XML；
* 路径全部由 __file__ 推导，换机器不用改。
"""
import ctypes
import os
import sys
from ctypes import wintypes

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')

sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')

import PyMfcXml as M  # noqa: E402

user32 = ctypes.WinDLL('user32', use_last_error=True)
user32.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
user32.SendMessageW.restype = ctypes.c_ssize_t
user32.GetDlgCtrlID.argtypes = [wintypes.HWND]
user32.GetDlgCtrlID.restype = ctypes.c_int
user32.GetClassNameW.argtypes = [wintypes.HWND, ctypes.c_wchar_p, ctypes.c_int]
user32.EnumChildWindows.argtypes = [wintypes.HWND, ctypes.c_void_p, wintypes.LPARAM]

WM_COMMAND = 0x0111
BN_CLICKED = 0

FAILS = []
CHECKS = [0]


def check(cond, what, got=None):
    CHECKS[0] += 1
    line = ('PASS ' if cond else 'FAIL ') + what
    if got is not None:
        line += '  got=%r' % (got,)
    print(line)
    if not cond:
        FAILS.append(what)
    return bool(cond)


def finish(title):
    print('\n%s: PASS=%d FAIL=%d' % (title, CHECKS[0] - len(FAILS), len(FAILS)))
    if FAILS:
        for f in FAILS:
            print('  - ' + f)
    return 1 if FAILS else 0


def children(root):
    """枚举后代窗口 -> {控件id: (hwnd, 类名)}。"""
    found = {}
    proto = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

    def cb(hwnd, _lparam):
        cid = user32.GetDlgCtrlID(hwnd)
        buf = ctypes.create_unicode_buffer(64)
        user32.GetClassNameW(hwnd, buf, 64)
        found[cid] = (hwnd, buf.value)
        return True

    user32.EnumChildWindows(root, proto(cb), 0)
    return found


def hwnd_of(win):
    h = win.hwnd() if callable(win.hwnd) else win.hwnd
    return h


def host_hwnd(win):
    """标准控件的父窗口（id=0 那个原生窗口）；WM_COMMAND 必须发给它。"""
    kids = children(hwnd_of(win))
    return (kids.get(0) or (hwnd_of(win), ''))[0]


def click(win, cid):
    """模拟一次按钮点击（和真实鼠标同一条 WM_COMMAND/BN_CLICKED）。"""
    kids = children(hwnd_of(win))
    ctl = (kids.get(cid) or (0, ''))[0]
    user32.SendMessageW(host_hwnd(win), WM_COMMAND, (BN_CLICKED << 16) | (cid & 0xFFFF),
                        ctl or 0)


user32.GetWindowRect.argtypes = [wintypes.HWND, ctypes.c_void_p]


def rect_of(win, cid):
    """真实窗口矩形 (x, y, w, h)，相对窗口左上角——布局对不对只能这么量。"""
    kids = children(hwnd_of(win))
    hwnd = (kids.get(cid) or (0, ''))[0]
    if not hwnd:
        return None
    r = wintypes.RECT()
    user32.GetWindowRect(hwnd, ctypes.byref(r))
    base = wintypes.RECT()
    user32.GetWindowRect(hwnd_of(win), ctypes.byref(base))
    return r.left - base.left, r.top - base.top, r.right - r.left, r.bottom - r.top
