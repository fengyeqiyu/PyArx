# -*- coding: utf-8 -*-
"""跨进程只读诊断：对现有演示窗口逐个发查询消息，判断控件是否真的装得下内容/状态。"""
import ctypes
import os
from ctypes import wintypes

user32 = ctypes.windll.user32
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, 'diag_existing.log')

WM_GETTEXTLENGTH = 0x000E
BM_GETCHECK = 0x00F0
LB_GETCOUNT = 0x018B
CB_GETCOUNT = 0x0146
TCM_GETITEMCOUNT = 0x1304
TVM_GETNEXTITEM = 0x110A
TVGN_ROOT = 0
LVM_GETITEMCOUNT = 0x1004
LVM_GETCOLUMNWIDTH = 0x101D
PBM_GETPOS = 0x0408
TBM_GETPOS = 0x0400
BST_CHECKED = 1

EnumProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
lines = []


def out(t):
    lines.append(t)
    print(t, flush=True)


def find_windows():
    found = []

    def cb(h, _):
        n = user32.GetWindowTextLengthW(h)
        if n > 0:
            buf = ctypes.create_unicode_buffer(n + 2)
            user32.GetWindowTextW(h, buf, n + 2)
            if buf.value[:2].isdigit() and ' - ' in buf.value:
                found.append((buf.value, h))
        return True

    user32.EnumWindows(EnumProc(cb), 0)
    return found


def cls_of(h):
    b = ctypes.create_unicode_buffer(128)
    user32.GetClassNameW(h, b, 128)
    return b.value


def child_by_id(top, cid):
    found = []

    def cb(c, _):
        if user32.GetDlgCtrlID(c) == cid:
            found.append(c)
            return False
        return True

    user32.EnumChildWindows(top, EnumProc(cb), 0)
    return found[0] if found else 0


def sm(h, msg, wp=0, lp=0):
    return user32.SendMessageW(h, msg, wp, lp)


CHECKS = {
    'TabControl': [('页签数 TCM_GETITEMCOUNT', TCM_GETITEMCOUNT)],
    'TreeCtrl': [('根节点 TVM_GETNEXTITEM', TVM_GETNEXTITEM)],
    'ListCtrl': [('行数 LVM_GETITEMCOUNT', LVM_GETITEMCOUNT),
                 ('列0宽 LVM_GETCOLUMNWIDTH', LVM_GETCOLUMNWIDTH)],
    'GridCtrl': [('行数 LVM_GETITEMCOUNT', LVM_GETITEMCOUNT),
                 ('列0宽 LVM_GETCOLUMNWIDTH', LVM_GETCOLUMNWIDTH)],
    'ListBox': [('项数 LB_GETCOUNT', LB_GETCOUNT)],
    'ComboBox': [('项数 CB_GETCOUNT', CB_GETCOUNT)],
    'Choice': [('项数 CB_GETCOUNT', CB_GETCOUNT)],
    'ProgressBar': [('位置 PBM_GETPOS', PBM_GETPOS)],
    'Slider': [('位置 TBM_GETPOS', TBM_GETPOS)],
}
CHECK_BUTTON = ('CheckBox', 'RadioButton', 'ToggleButton')


def main():
    wins = find_windows()
    out('找到演示窗口 %d 个' % len(wins))
    for title, h in sorted(wins):
        tag = title.split(' ')[1]
        child = child_by_id(h, 1000)
        if not child:
            out('%s  ->  id=1000 不存在！' % title)
            continue
        cls = cls_of(child)
        vis = bool(user32.IsWindowVisible(child))
        enabled = bool(user32.IsWindowEnabled(child))
        textlen = sm(child, WM_GETTEXTLENGTH)
        notes = []
        if tag in CHECK_BUTTON:
            notes.append('勾选 BM_GETCHECK=%d' % sm(child, BM_GETCHECK))
        for label, msg in CHECKS.get(tag, []):
            notes.append('%s=%d' % (label, sm(child, msg)))
        out('%-34s class=%-18s visible=%d enabled=%d 文本长度=%d  %s'
            % (title, cls, vis, enabled, textlen, ' | '.join(notes)))

    with open(OUT, 'w', encoding='utf-8') as fh:
        fh.write('\n'.join(lines) + '\n')


if __name__ == '__main__':
    main()
