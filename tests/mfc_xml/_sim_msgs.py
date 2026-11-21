# -*- coding: utf-8 -*-
"""模拟消息测试（不用真实鼠标键盘）：用 SendMessage 把 WM_COMMAND / WM_KEY* /
WM_LBUTTON* 直接投给宿主与控件，验证 S2 事件绑定到底接没接上。

覆盖：
  TextChanged      <- 对 EDIT 发 WM_SETTEXT，控件回送 EN_CHANGE
  Click            <- WM_COMMAND(id, BN_CLICKED)
  DoubleClick      <- WM_COMMAND(id, BN_DOUBLECLICKED)
  SelectionChanged <- COMBOBOX 的 CBN_SELCHANGE
  DropDown         <- COMBOBOX 的 CBN_DROPDOWN
  ValueChanged     <- WM_HSCROLL（lParam = 控件 HWND）
  KeyDown/KeyUp    <- WM_KEYDOWN / WM_KEYUP
  GotFocus         <- WM_SETFOCUS
  MouseDown/Up     <- WM_LBUTTONDOWN / WM_LBUTTONUP（控件子类化）
  CheckedChanged   <- 复选框 BN_CLICKED 分支
"""
import ctypes
import os
import sys
from ctypes import wintypes

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
os.environ.setdefault('PYARX_D2D_RT', 'software')

import PyMfcXml as M

user32 = ctypes.WinDLL('user32', use_last_error=True)
user32.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
user32.SendMessageW.restype = ctypes.c_ssize_t
user32.GetClassNameW.argtypes = [wintypes.HWND, ctypes.c_wchar_p, ctypes.c_int]
user32.GetDlgCtrlID.argtypes = [wintypes.HWND]
user32.GetDlgCtrlID.restype = ctypes.c_int
user32.EnumChildWindows.argtypes = [wintypes.HWND, ctypes.c_void_p, wintypes.LPARAM]
user32.GetClientRect.argtypes = [wintypes.HWND, ctypes.c_void_p]

WM_COMMAND = 0x0111
WM_SETTEXT = 0x000C
WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
WM_SETFOCUS = 0x0007
WM_LBUTTONDOWN = 0x0201
WM_LBUTTONUP = 0x0202
WM_MOUSEMOVE = 0x0200
WM_HSCROLL = 0x0114

BN_CLICKED = 0
BN_DOUBLECLICKED = 5
EN_CHANGE = 0x0300
CBN_SELCHANGE = 1
CBN_DROPDOWN = 7
MK_LBUTTON = 0x0001

HITS = {}


# 宿主派发回调时不传位置参数，统一用 *a 兼容（需要负载的再单独看）
def onClick(*a):
    HITS['click'] = HITS.get('click', 0) + 1


def onDbl(*a):
    HITS['dbl'] = HITS.get('dbl', 0) + 1


def onText(*a):
    HITS['text'] = HITS.get('text', 0) + 1


def onSel(*a):
    HITS['sel'] = HITS.get('sel', 0) + 1


def onValue(*a):
    HITS['value'] = HITS.get('value', 0) + 1


def onChecked(*a):
    HITS['checked'] = HITS.get('checked', 0) + 1


def onDropDown(*a):
    HITS['dropdown'] = HITS.get('dropdown', 0) + 1


def onCtlKeyDown(*a):
    HITS['ctl_keydown'] = HITS.get('ctl_keydown', 0) + 1


def onCtlKeyUp(*a):
    HITS['ctl_keyup'] = HITS.get('ctl_keyup', 0) + 1


def onCtlFocus(*a):
    HITS['ctl_gotfocus'] = HITS.get('ctl_gotfocus', 0) + 1


def onCtlMouseDown(*a):
    HITS['ctl_mousedown'] = HITS.get('ctl_mousedown', 0) + 1


def onCtlMouseUp(*a):
    HITS['ctl_mouseup'] = HITS.get('ctl_mouseup', 0) + 1


XML = '''<XWindow Title="模拟消息测试" Width="620" Height="420">
  <XTextField Name="T1" Left="16" Top="16" Width="240" Height="28"
           Text="初始" Click="onClick" DoubleClick="onDbl" TextChanged="onText"
           KeyDown="onCtlKeyDown" KeyUp="onCtlKeyUp" MouseDown="onCtlMouseDown" MouseUp="onCtlMouseUp"
           GotFocus="onCtlFocus"/>
  <XCheckBox Name="C1" Left="16" Top="56" Width="140" Height="26"
            Text="勾我" Click="onClick" CheckedChanged="onChecked"/>
  <XComboBox Name="CB1" Left="16" Top="92" Width="200" Height="26"
            Items="甲|乙|丙" SelectedIndex="0"
            SelectionChanged="onSel" DropDown="onDropDown"/>
  <XSlider Name="S1" Left="16" Top="130" Width="200" Height="24"
          Minimum="0" Maximum="10" ValueChanged="onValue"/>
</XWindow>'''

fails = []


def check(cond, what, got=None):
    line = ('OK   ' if cond else 'FAIL ') + what
    if got is not None:
        line += '  got=%r' % (got,)
    print(line)
    if not cond:
        fails.append(what)


def children(root):
    """枚举所有后代窗口，返回 {控件id: (hwnd, 类名)}。"""
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


def cmd(hwnd, cid, code, ctl_hwnd=0):
    user32.SendMessageW(hwnd, WM_COMMAND, (code << 16) | (cid & 0xFFFF), ctl_hwnd)


_ALIVE = []      # 缓冲区必须活到 SendMessage 返回，否则地址悬空、文本变乱码


def wstr(s):
    """宽字符串 -> LPARAM（SendMessage 的 LPARAM 声明成整数，不能直接传 c_wchar_p）。"""
    buf = ctypes.create_unicode_buffer(s)
    _ALIVE.append(buf)
    return ctypes.addressof(buf)


def main():
    path = r'G:\PyArx\docs\_sim_msgs.xml'
    with open(path, 'w', encoding='utf-8') as f:
        f.write(XML)

    names = {}
    win = M.guiload(path, context=globals(), names=names)
    win.show()
    M.pump(150)

    hw = win.hwnd() if callable(win.hwnd) else win.hwnd
    kids = children(hw)
    print('子窗口: %d 个' % len(kids))
    for cid, (h, cls) in sorted(kids.items()):
        print('  id=%-5d cls=%s' % (cid, cls))

    # 所有标准控件的父窗口是宿主的 PyMfcNativeWindow（id=0），
    # WM_COMMAND / WM_HSCROLL 必须发给它，发给外层框架没用
    host_hw = kids.get(0, (hw, ''))[0]
    print('宿主窗口 = %#x' % (host_hw or 0))

    tid, cid_, bid, sid = names['T1'], names['C1'], names['CB1'], names['S1']
    th = kids.get(tid, (0, ''))[0]
    ch = kids.get(cid_, (0, ''))[0]
    bh = kids.get(bid, (0, ''))[0]

    # 1) TextChanged：EDIT 收到 WM_SETTEXT 后会回送 EN_CHANGE
    if th:
        user32.SendMessageW(th, WM_SETTEXT, 0, wstr('灌入'))
        check(HITS.get('text', 0) >= 1, 'TextChanged 触发', HITS.get('text'))
        v = win.get_xml_value(tid, 'text')
        check(v == '灌入', 'TextChanged 后宿主文本已更新', v)
    else:
        check(False, '找不到 TextBox 子窗口')

    # 2) Click / DoubleClick：直接发 BN_CLICKED / BN_DOUBLECLICKED（与真实按钮同一条 WM_COMMAND）
    base = dict(HITS)
    cmd(host_hw, tid, BN_CLICKED, th)
    check(HITS.get('click', 0) == base.get('click', 0) + 1, 'Click 触发', HITS.get('click'))
    base = dict(HITS)
    cmd(host_hw, tid, BN_DOUBLECLICKED, th)
    check(HITS.get('dbl', 0) == base.get('dbl', 0) + 1, 'DoubleClick 触发', HITS.get('dbl'))

    # 3) 复选框：BN_CLICKED 若控件是 BS_AUTOCHECKBOX 类样式，还要顺带发 CheckedChanged
    if ch:
        base = dict(HITS)
        user32.SendMessageW(ch, 0x00F5, 0, 0)   # BM_CLICK -> 控件自己发 BN_CLICKED
        check(HITS.get('click', 0) == base.get('click', 0) + 1, 'CheckBox Click 触发', HITS.get('click'))
        check(HITS.get('checked') is not None, 'CheckBox CheckedChanged 触发', HITS.get('checked'))
    else:
        check(False, '找不到 CheckBox 子窗口')

    # 4) ComboBox：选第 2 项后发 CBN_SELCHANGE / CBN_DROPDOWN
    if bh:
        user32.SendMessageW(bh, 0x014E, 1, 0)   # CB_SETCURSEL
        cmd(host_hw, bid, CBN_SELCHANGE, bh)
        check(HITS.get('sel') is not None, 'SelectionChanged 触发', HITS.get('sel'))
        base = dict(HITS)
        cmd(host_hw, bid, CBN_DROPDOWN, bh)
        check(HITS.get('dropdown', 0) == base.get('dropdown', 0) + 1, 'DropDown 触发', HITS.get('dropdown'))
    else:
        check(False, '找不到 ComboBox 子窗口')

    # 5) KeyDown / KeyUp / GotFocus / 鼠标：走控件子类化
    if th:
        base = dict(HITS)
        user32.SendMessageW(th, WM_KEYDOWN, 0x41, 1)
        check(HITS.get('ctl_keydown', 0) == base.get('ctl_keydown', 0) + 1,
              'KeyDown 触发', HITS.get('ctl_keydown'))
        user32.SendMessageW(th, WM_KEYUP, 0x41, 0xC0000001)
        check(HITS.get('ctl_keyup', 0) == base.get('ctl_keyup', 0) + 1,
              'KeyUp 触发', HITS.get('ctl_keyup'))
        user32.SendMessageW(th, WM_SETFOCUS, 0, 0)
        check(HITS.get('ctl_gotfocus', 0) == base.get('ctl_gotfocus', 0) + 1,
              'GotFocus 触发', HITS.get('ctl_gotfocus'))
        lp = (20 << 16) | 60          # 客户坐标 (60, 20)
        user32.SendMessageW(th, WM_LBUTTONDOWN, MK_LBUTTON, lp)
        user32.SendMessageW(th, WM_LBUTTONUP, 0, lp)
        check(HITS.get('ctl_mousedown', 0) == base.get('ctl_mousedown', 0) + 1,
              'MouseDown 触发', HITS.get('ctl_mousedown'))
        check(HITS.get('ctl_mouseup', 0) == base.get('ctl_mouseup', 0) + 1,
              'MouseUp 触发', HITS.get('ctl_mouseup'))

    # 6) ValueChanged：WM_HSCROLL（lParam 传控件句柄，正是宿主 OnHScroll 的取法）
    sh = kids.get(sid, (0, ''))[0]
    if sh:
        base = dict(HITS)
        user32.SendMessageW(host_hw, WM_HSCROLL, 4, sh)   # SB_THUMBPOSITION
        check(HITS.get('value') is not None, 'ValueChanged 触发', HITS.get('value'))
    else:
        print('（Slider 没有独立子窗口，跳过 WM_HSCROLL）')

    print('HITS=%r' % HITS)
    win.save_png(r'G:\PyArx\docs\_sim_msgs.png')
    win.close()
    print('\n结果：%d 项失败' % len(fails))
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
