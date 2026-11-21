# -*- coding: utf-8 -*-
"""uiauto —— cad2d 界面自动化的最小工具集（真实 HWND + 真实点击）。

为什么不用 win.hwnd：
    PyMfcXml 的 win.hwnd 是框架内部的窗口 id，不是 Win32 HWND。
    把 WM_COMMAND 发给它什么都不会发生。必须用 EnumChildWindows 按控件 id
    找真实句柄，再给控件自己发 BM_CLICK（按钮类控件的点击派发已验证走这条）。

用法：
    import uiauto
    uiauto.click(win, ids["tbLINE"])      # 触发 on_click 绑定
    uiauto.children(win)                  # [(hwnd, ctrl_id), ...]
"""
import ctypes
import ctypes.wintypes as wt

_u = ctypes.windll.user32
_u.GetDlgCtrlID.argtypes = [ctypes.c_void_p]
_u.GetDlgCtrlID.restype = ctypes.c_int
_u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.POINTER(wt.RECT)]
_u.SendMessageW.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p]
_u.SendMessageW.restype = ctypes.c_void_p

BM_CLICK = 0x00F5
WM_SETTEXT = 0x000C

_PROC = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p, ctypes.c_void_p)


def children(win):
    """窗口里所有子控件的 (hwnd, ctrl_id)。递归枚举，含容器里的控件。"""
    out = []

    def _cb(hw, _lp):
        out.append((hw, _u.GetDlgCtrlID(ctypes.c_void_p(hw))))
        return True

    _u.EnumChildWindows(ctypes.c_void_p(int(win.hwnd)),
                        ctypes.cast(_PROC(_cb), ctypes.c_void_p), None)
    return out


def hwnd_of(win, ctrl_id):
    for hw, cid in children(win):
        if cid == ctrl_id:
            return hw
    return None


def rect(win, ctrl_id):
    """控件屏幕矩形 (x, y, w, h)；控件不在时抛 ValueError。"""
    hw = hwnd_of(win, ctrl_id)
    if hw is None:
        raise ValueError('no hwnd for ctrl id %s' % ctrl_id)
    rc = wt.RECT()
    _u.GetWindowRect(ctypes.c_void_p(hw), ctypes.byref(rc))
    return rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top


def click(win, ctrl_id):
    """真实点击（BM_CLICK）：按钮类控件走到 Python 侧 on_click 回调。"""
    hw = hwnd_of(win, ctrl_id)
    if hw is None:
        raise ValueError('no hwnd for ctrl id %s' % ctrl_id)
    _u.SendMessageW(ctypes.c_void_p(hw), BM_CLICK, None, None)


class Checker(object):
    """断言收集器：跑完全部用例再判，失败带上下文，不半路抛。"""

    def __init__(self):
        self.rows = []

    def add(self, case, ok, detail=''):
        self.rows.append((bool(ok), case, detail))
        print('%-4s %-34s %s' % ('PASS' if ok else 'FAIL', case, detail))
        return ok

    def eq(self, case, actual, expected, detail=''):
        ok = actual == expected
        return self.add(case, ok, '%s | 期望 %r 实际 %r' % (detail, expected, actual) if not ok else detail)

    @property
    def failed(self):
        return [r for r in self.rows if not r[0]]

    def done(self):
        bad = self.failed
        print('\n%d/%d PASS' % (len(self.rows) - len(bad), len(self.rows)))
        assert not bad, '失败 %d 项：%s' % (len(bad), '; '.join(r[1] for r in bad))
        return True
