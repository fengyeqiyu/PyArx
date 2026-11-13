# -*- coding: utf-8 -*-
"""_verify_anim.py — 动画窗体验收（纯 Python，独立进程，不碰 CAD/ARX）。

判定故意不依赖"墙上时钟正好 300ms"——脚本 pump 的步长有抖动，
所以宽度/数值只看**单调推进 + 端点到位**，颜色按控件矩形内取最饱和像素。
"""
import ctypes
import os
import struct
from ctypes import wintypes

import PyMfcXaml as p
from _verify_flickable import capture  # 离屏合成抓图

u = ctypes.windll.user32
u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.GetDlgItem.argtypes = [ctypes.c_void_p, ctypes.c_int]
u.GetDlgItem.restype = ctypes.c_void_p
u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
CB = ctypes.WINFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)


class R(ctypes.Structure):
    _fields_ = [('l', ctypes.c_long), ('t', ctypes.c_long), ('r', ctypes.c_long), ('b', ctypes.c_long)]


XAML = '''<?xml version="1.0" encoding="utf-8"?>
<Window xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="动画验收" Width="560" Height="380">
  <Layout x:Name="root">
    <TextBlock x:Name="t1" Text="PropertyAnimation：按钮 Width" Width="460" Height="26" Canvas.Left="24" Canvas.Top="18" />
    <Button x:Name="bt" Width="140" Height="44" Content="宽度动画" Canvas.Left="24" Canvas.Top="52" />
    <TextBlock x:Name="t2" Text="NumberAnimation：SpinBox Value" Width="460" Height="26" Canvas.Left="24" Canvas.Top="120" />
    <SpinBox x:Name="sp" Width="220" Height="34" Value="0" Minimum="0" Maximum="99" Canvas.Left="24" Canvas.Top="152" />
    <TextBlock x:Name="t3" Text="ColorAnimation：Switch 底色" Width="460" Height="26" Canvas.Left="24" Canvas.Top="206" />
    <Switch x:Name="sw" Width="120" Height="45" Value="1" Canvas.Left="24" Canvas.Top="238" />
  </Layout>
</Window>'''

PATH = os.path.join(r'G:\PyArx\tests\mfc_controls', '_anim_verify.xaml')


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


def geom(hwnd):
    r = R()
    u.GetWindowRect(ctypes.c_void_p(hwnd), ctypes.byref(r))
    return (r.l, r.t, r.r - r.l, r.b - r.t)


def read_bmp(path):
    d = open(path, 'rb').read()
    off = struct.unpack_from('<I', d, 10)[0]
    w = struct.unpack_from('<i', d, 18)[0]
    h = struct.unpack_from('<i', d, 22)[0]
    bpp = struct.unpack_from('<H', d, 28)[0]
    stride = ((w * bpp // 8 + 3) // 4) * 4
    topdown = h < 0
    hh = abs(h)

    def px(x, y):
        row = y if topdown else (hh - 1 - y)
        i = off + row * stride + x * 4
        return (d[i + 2], d[i + 1], d[i])

    return px, w, hh


def most_bluish(path, box):
    px, w, hh = read_bmp(path)
    best = None
    l, t, r, b = box
    for y in range(max(0, t), min(hh, b), 6):
        for x in range(max(0, l), min(w, r), 6):
            c = px(x, y)
            score = c[2] - c[0]
            if best is None or score > best[0]:
                best = (score, c)
    return best


def pump_ms(ms, step=20):
    spent = 0
    while spent < ms:
        p.pump(step)
        spent += step


def main():
    with open(PATH, 'w', encoding='utf-8') as f:
        f.write(XAML)
    names = {}
    w = p.guiload(PATH, names=names)
    w.show()
    p.pump(200)
    bt, sp, sw = names['bt'], names['sp'], names['sw']
    hbt, hsw = find_child(w, bt), find_child(w, sw)
    assert hbt and hsw, '子窗 hwnd 找不到'

    ok = bad = 0

    def check(name, good, detail):
        nonlocal ok, bad
        ok, bad = (ok + 1, bad) if good else (ok, bad + 1)
        print(('  OK   ' if good else '  FAIL ') + '%-34s %s' % (name, detail))

    # ---- 1) PropertyAnimation：140 → 420（linear）
    wid0 = geom(hbt)[2]
    check('起始宽度 140', abs(wid0 - 140) <= 4, 'got=%d' % wid0)
    w.animate(bt, 'Width', 420, 900, 'linear')
    widths = []
    for _ in range(14):
        pump_ms(70)
        widths.append(geom(hbt)[2])
    mono = all(widths[i] <= widths[i + 1] + 1 for i in range(len(widths) - 1))
    check('宽度单调推进', mono, '采样=%s' % (widths,))
    check('宽度到点 420', abs(widths[-1] - 420) <= 6, 'got=%d' % widths[-1])

    # ---- 2) NumberAnimation：0 → 99（outCubic）
    w.setSpin(sp, 0)
    p.pump(60)
    w.animateFrom(sp, 'Value', 0, 99, 750, 'outCubic', 0, False)
    vals = []
    for _ in range(12):
        pump_ms(80)
        vals.append(w.getSpin(sp))
    mono_v = all(vals[i] <= vals[i + 1] + 1 for i in range(len(vals) - 1))
    check('数值单调推进', mono_v, '采样=%s' % (vals,))
    check('数值到点 99', vals[-1] >= 98, 'got=%d' % vals[-1])
    fast_start = vals[1] >= 30 if len(vals) > 1 else False
    check('outCubic 前快后慢', fast_start, '第 2 个采样(约160ms)=%d' % (vals[1] if len(vals) > 1 else 0))

    # ---- 3) ColorAnimation：灰 → 强调蓝（按控件矩形取最饱和像素）
    wr = geom(w.hwnd)
    swr = geom(hsw)
    box = (swr[0] - wr[0], swr[1] - wr[1], swr[0] - wr[0] + swr[2], swr[1] - wr[1] + swr[3])
    capture(w.hwnd, r'G:\PyArx\docs\_an_before.bmp')
    b0 = most_bluish(r'G:\PyArx\docs\_an_before.bmp', box)
    w.animateColor(sw, 0x00D27800, 900, 'outCubic', 0)
    pump_ms(300)
    capture(w.hwnd, r'G:\PyArx\docs\_an_mid.bmp')
    b1 = most_bluish(r'G:\PyArx\docs\_an_mid.bmp', box)
    pump_ms(700)
    capture(w.hwnd, r'G:\PyArx\docs\_an_end.bmp')
    b2 = most_bluish(r'G:\PyArx\docs\_an_end.bmp', box)
    print('    Switch 最蓝像素：before=%s mid=%s end=%s' % (b0, b1, b2))
    check('颜色动画 起点偏灰', b0[0] <= 40, 'B-R=%d' % b0[0])
    check('颜色动画 中间帧过渡', b1[0] > b0[0] + 15, 'B-R=%d' % b1[0])
    check('颜色动画 终点强调蓝', b2[0] > 120 and b2[1][0] < 80, 'px=%s' % (b2[1],))

    # ---- 4) 回程不漂移
    w.setSpin(sp, 99)
    p.pump(60)
    w.animateFrom(sp, 'Value', 99, 0, 400, 'linear', 0, False)
    pump_ms(500)
    check('回程 Value 回 0', w.getSpin(sp) <= 1, 'got=%d' % w.getSpin(sp))
    check('Width 停在 420', abs(geom(hbt)[2] - 420) <= 6, 'got=%d' % geom(hbt)[2])

    print('动画验收：通过 %d，失败 %d' % (ok, bad))
    print('ANIM TEST %s' % ('PASSED' if bad == 0 else 'FAILED'))
    w.close()
    p.pump(60)


if __name__ == '__main__':
    main()
