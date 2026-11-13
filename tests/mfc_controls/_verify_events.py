# -*- coding: utf-8 -*-
"""_verify_events.py — 全控件事件驱动审计（纯 Python，独立进程）。

做法：把 26 类控件全建出来，给**每一个**控件挂满 7 个通道的回调
(onClick / onDoubleClick / onChange / onSelect / onDropDown / onValue / onChecked)，
再把真实鼠标/键盘消息合成到控件 HWND 上，记录哪个通道真的被点亮：

  · 单击      WM_LBUTTONDOWN + WM_LBUTTONUP
  · 双击      WM_LBUTTONDBLCLK + WM_LBUTTONUP（连续两次）
  · 悬停      WM_MOUSEMOVE（光标停住）/ WM_MOUSELEAVE
  · 滚轮      WM_MOUSEWHEEL（±120）
  · 右键      WM_RBUTTONDOWN + WM_RBUTTONUP
  · 键盘      WM_CHAR

输出一张 控件 × 事件 的矩阵，并打印"一个通道都没被点亮"的控件清单。
"""
import ctypes
import os
from ctypes import wintypes

import PyMfcXaml as p

HERE = os.path.dirname(os.path.abspath(__file__))
u = ctypes.windll.user32

u.SendMessageW.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p]
u.SendMessageW.restype = ctypes.c_void_p
u.PostMessageW.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p]
u.GetClientRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.ClientToScreen.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
CB = ctypes.WINFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)

WM_LBUTTONDOWN = 0x0201
WM_LBUTTONUP = 0x0202
WM_LBUTTONDBLCLK = 0x0203
WM_RBUTTONDOWN = 0x0204
WM_RBUTTONUP = 0x0205
WM_MOUSEMOVE = 0x0200
WM_MOUSELEAVE = 0x02A3
WM_MOUSEWHEEL = 0x020A
WM_CHAR = 0x0102


class R(ctypes.Structure):
    _fields_ = [('l', ctypes.c_long), ('t', ctypes.c_long), ('r', ctypes.c_long), ('b', ctypes.c_long)]


class P(ctypes.Structure):
    _fields_ = [('x', ctypes.c_long), ('y', ctypes.c_long)]


EVENTS = ['click', 'dblclick', 'change', 'select', 'dropdown', 'value', 'checked']


def enum_all(top):
    """id -> (hwnd, class, rect)"""
    found = {}

    def cb(h, _):
        cid = u.GetDlgCtrlID(ctypes.c_void_p(h))
        b = ctypes.create_unicode_buffer(128)
        u.GetClassNameW(ctypes.c_void_p(h), b, 128)
        r = R()
        u.GetWindowRect(ctypes.c_void_p(h), ctypes.byref(r))
        if cid not in found:
            found[cid] = (h, b.value, (r.l, r.t, r.r, r.b))
        return 1

    u.EnumChildWindows(ctypes.c_void_p(top), CB(cb), 0)
    return found


def center_topdown(h, fx=0.5, fy=0.5):
    """控件内的相对位置 → (client_lparam, screen_lparam)。
    下拉框要点在文字区（20% 处）：点右端箭头会弹出模态列表，把泵卡住。
    """
    cr = R()
    u.GetClientRect(ctypes.c_void_p(h), ctypes.byref(cr))
    cx, cy = int((cr.r - cr.l) * fx), int((cr.b - cr.t) * fy)
    pt = P(cx, cy)
    u.ClientToScreen(ctypes.c_void_p(h), ctypes.byref(pt))
    lp_client = ((cy & 0xFFFF) << 16) | (cx & 0xFFFF)
    lp_screen = ((pt.y & 0xFFFF) << 16) | (pt.x & 0xFFFF)
    return lp_client, lp_screen


def send(h, msg, wp=0, lp=0):
    u.PostMessageW(ctypes.c_void_p(h), msg, ctypes.c_void_p(wp & 0xFFFFFFFF), ctypes.c_void_p(lp & 0xFFFFFFFF))


def click_xy(h, cls=''):
    fx = 0.2 if cls == 'ComboBox' else 0.5
    lp, _ = center_topdown(h, fx)
    send(h, WM_LBUTTONDOWN, 1, lp)
    send(h, WM_LBUTTONUP, 0, lp)


def synth_click(h, cls=''):
    click_xy(h, cls)
    p.pump(60)


def synth_dblclick(h, cls=''):
    fx = 0.2 if cls == 'ComboBox' else 0.5
    lp, _ = center_topdown(h, fx)
    for _ in range(2):
        send(h, WM_LBUTTONDBLCLK, 1, lp)
        send(h, WM_LBUTTONUP, 0, lp)
        p.pump(30)


def synth_hover(h):
    lp, _ = center_topdown(h)
    for _ in range(4):
        send(h, WM_MOUSEMOVE, 0, lp)
        p.pump(20)
    send(h, WM_MOUSELEAVE, 0, 0)
    p.pump(30)


def synth_wheel(h, delta=120):
    _, lp = center_topdown(h)
    wp = ((delta & 0xFFFF) << 16)
    send(h, WM_MOUSEWHEEL, wp, lp)
    p.pump(30)


def synth_right(h):
    lp, _ = center_topdown(h)
    send(h, WM_RBUTTONDOWN, 2, lp)
    send(h, WM_RBUTTONUP, 0, lp)
    p.pump(30)


def synth_key(h):
    send(h, WM_CHAR, ord('K'), 0)
    p.pump(30)


def main():
    log = {}
    names = {}

    def make(tag, cid):
        # 回调实际是零参调用，所以 cid 必须在闭包里绑死，不能靠参数拿
        def fn(*a):
            log.setdefault(cid, set()).add(tag)
        return fn

    def audit(xaml, expect_tag):
        nm = {}
        w = p.guiload(os.path.join(HERE, xaml), names=nm)
        w.show()
        p.pump(350)
        ids = enum_all(w.hwnd)
        for ctl_name, cid in nm.items():
            if cid not in ids:
                continue
            h = ids[cid][0]
            w.onClick(cid, make('click', cid))
            w.onDoubleClick(cid, make('dblclick', cid))
            w.onChange(cid, make('change', cid))
            w.onSelect(cid, make('select', cid))
            w.onDropDown(cid, make('dropdown', cid))
            w.onValue(cid, make('value', cid))
            w.onChecked(cid, make('checked', cid))
            cls = ids[cid][1]
            synth_click(h, cls)
            synth_dblclick(h, cls)
            synth_hover(h)
            synth_wheel(h, 120)
            synth_wheel(h, -120)
            synth_right(h)
            synth_key(h)
            p.pump(60)
        for ctl_name, cid in nm.items():
            if cid in ids:
                names[ctl_name] = (ids[cid][1], set(log.get(cid, set())), ids[cid][2])
        return w

    w1 = audit('controls_basic.xaml', 'basic')
    w2 = audit('controls_extended.xaml', 'ext')

    print('\n== 事件矩阵（控件 / 原生类 / 被点亮的通道）==')
    for name, (cls, fired, rect) in sorted(names.items()):
        ww, hh = rect[2] - rect[0], rect[3] - rect[1]
        mark = ','.join(sorted(fired)) if fired else '（无）'
        print('  %-14s %-22s %4dx%-4d  %s' % (name, cls, ww, hh, mark))

    dead = [n for n, v in names.items() if not v[1]]
    print('\n一个通道都没点亮的控件：%s' % (dead if dead else '无'))
    print('总计 %d 个控件；有事件响应的 %d 个' % (len(names), len(names) - len(dead)))
    for w in (w1, w2):
        w.close()
    p.pump(60)


if __name__ == '__main__':
    main()
