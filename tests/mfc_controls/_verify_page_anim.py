# -*- coding: utf-8 -*-
"""_verify_page_anim.py — "每页一个动画开关"验收（纯 Python，独立进程）。

验六条：开关能起停本页动画；切页会把离开那页的动画停掉；
回到那页开关还开着就自动续跑；没打开的页不会跟着跑。
"""
import ctypes
import os
from ctypes import wintypes

import PyMfcXaml as p
from demo_page_anim import PageAnim, build

u = ctypes.windll.user32
u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.GetDlgItem.argtypes = [ctypes.c_void_p, ctypes.c_int]
u.GetDlgItem.restype = ctypes.c_void_p
u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
CB = ctypes.WINFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)


class R(ctypes.Structure):
    _fields_ = [('l', ctypes.c_long), ('t', ctypes.c_long), ('r', ctypes.c_long), ('b', ctypes.c_long)]


def find_child(w, cid):
    got = []

    def on(h, _):
        got.append(h)
        return 1

    u.EnumChildWindows(ctypes.c_void_p(w.hwnd), CB(on), 0)
    for h in got:
        h2 = u.GetDlgItem(ctypes.c_void_p(h), cid)
        if h2:
            return h2
    return None


def width_of(hwnd):
    r = R()
    u.GetWindowRect(ctypes.c_void_p(hwnd), ctypes.byref(r))
    return r.r - r.l


def pump_ms(ms, step=20, tick=None):
    spent = 0
    while spent < ms:
        p.pump(step)
        spent += step
        if tick:
            tick()


def settle_width(fn, ms=600):
    """连续 3 次读数不变再返回，避免把"还在动"当成"已停"。"""
    last = None
    stable = 0
    spent = 0
    while spent < ms:
        p.pump(20)
        spent += 20
        cur = fn()
        stable = stable + 1 if cur == last else 0
        last = cur
        if stable >= 3:
            break
    return last


def main():
    names = {}
    w = p.guiload(build(), names=names)
    w.show()
    p.pump(250)
    sv = names['sv']
    w.addPage(sv, [names['s1'], names['p1t'], names['p1b'], names['p1p']])
    w.addPage(sv, [names['s2'], names['p2t'], names['p2s']])
    w.addPage(sv, [names['s3'], names['p3t'], names['p3d']])
    pa = PageAnim(w, names)

    hb = find_child(w, names['p1b'])
    assert hb, '按钮 hwnd 找不到'
    wid = lambda: width_of(hb)

    ok = bad = 0

    def check(name, good, detail):
        nonlocal ok, bad
        ok, bad = (ok + 1, bad) if good else (ok, bad + 1)
        print(('  OK   ' if good else '  FAIL ') + '%-38s %s' % (name, detail))

    pump_ms(200, tick=pa.tick)
    check('初始三页开关全关', all(w.getSwitch(x) == 0 for x in pa.sw),
          'switchs=%s' % [w.getSwitch(x) for x in pa.sw])
    check('初始停在第 1 页', w.getViewIndex(sv) == 0, 'index=%d' % w.getViewIndex(sv))
    check('初始宽度 140（静止）', abs(wid() - 140) <= 4, 'got=%d' % wid())

    # 1) 打开第 1 页开关 → 宽度开始走
    w.setSwitch(pa.sw[0], 1)
    seq = []
    for _ in range(6):
        pump_ms(120, tick=pa.tick)
        seq.append(wid())
    check('开开关后宽度在动', max(seq) - min(seq) >= 30, '采样=%s' % (seq,))

    # 2) 关掉 → 停住
    w.setSwitch(pa.sw[0], 0)
    pump_ms(300, tick=pa.tick)
    a = settle_width(wid)
    pump_ms(400, tick=pa.tick)
    b = settle_width(wid)
    check('关开关后动画停住', abs(a - b) <= 2, 'a=%d b=%d' % (a, b))

    # 3) 再打开，然后切到第 2 页 → 第 1 页动画必须停
    w.setSwitch(pa.sw[0], 1)
    pump_ms(240, tick=pa.tick)
    w.setViewIndex(sv, 1)
    pump_ms(300, tick=pa.tick)
    a2 = settle_width(wid)
    pump_ms(400, tick=pa.tick)
    b2 = settle_width(wid)
    check('切走后第 1 页动画停掉', abs(a2 - b2) <= 2, 'a=%d b=%d' % (a2, b2))
    check('已停在第 2 页', w.getViewIndex(sv) == 1, 'index=%d' % w.getViewIndex(sv))

    # 4) 第 2 页开关没打开 → 它的数值不该自己动
    v0 = w.getSpin(names['p2s'])
    pump_ms(600, tick=pa.tick)
    check('未开开关的页不跑动画', w.getSpin(names['p2s']) == v0, 'v=%d→%d' % (v0, w.getSpin(names['p2s'])))

    # 5) 打开第 2 页开关 → 数值开始走
    w.setSwitch(pa.sw[1], 1)
    vals = []
    for _ in range(5):
        pump_ms(140, tick=pa.tick)
        vals.append(w.getSpin(names['p2s']))
    check('第 2 页开关起效', max(vals) - min(vals) >= 10, '采样=%s' % (vals,))

    # 6) 切回第 1 页 → 它开关还开着，应自动续跑；同时第 2 页停
    w.setViewIndex(sv, 0)
    pump_ms(300, tick=pa.tick)
    seq2 = []
    for _ in range(5):
        pump_ms(120, tick=pa.tick)
        seq2.append(wid())
    check('回页后自动续跑', max(seq2) - min(seq2) >= 30, '采样=%s' % (seq2,))

    print('页面动画验收：通过 %d，失败 %d' % (ok, bad))
    print('PAGE-ANIM TEST %s' % ('PASSED' if bad == 0 else 'FAILED'))
    w.close()
    p.pump(60)


if __name__ == '__main__':
    main()
