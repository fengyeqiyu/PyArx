# -*- coding: utf-8 -*-
"""CSS `aspect-ratio` 必须能从 XML 的 style 一路落到真实盒尺寸。

缺口：`style="width:160;aspect-ratio:16/9"` 在 XML 侧完全没有通路 —— 写了 height 仍然
按默认高度，比例完全不生效。HTML 里图片/视频/封面卡片的标准写法，XML 只能自己算死
height，容器宽度一变比例就崩。

本轮打通：style aspect-ratio（16/9、2/1、1.5）→ AspectRatio 属性 → 普通流布局
"只补未声明的那一边"，两边都写以显式声明为准。
断言方式：背景色包围盒量宽高（160×90、240×120 这种精确对应）。
"""
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')

import PyMfcXml as M  # noqa: E402

FAILS = []
CHECKS = [0]
RGB = (0x2E, 0x7D, 0x32)


def check(cond, what, got=None):
    CHECKS[0] += 1
    print(('PASS ' if cond else 'FAIL ') + what + ('' if got is None else '  got=%r' % (got,)))
    if not cond:
        FAILS.append(what)
    return bool(cond)


def shot(xml, png):
    path = os.path.join(ROOT, 'log', '_t_aspect.xml')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(xml)
    win = M.guiload(path, context={}, names={})
    win.show()
    M.pump(400)
    win.save_png(png)
    win.close()
    from PIL import Image

    return Image.open(png).convert('RGB')


def bbox(img, rgb, tol=14):
    w, h = img.size
    xs, ys = [], []
    px = img.load()
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            if abs(r - rgb[0]) <= tol and abs(g - rgb[1]) <= tol and abs(b - rgb[2]) <= tol:
                xs.append(x)
                ys.append(y)
    if not xs:
        return None
    return (min(xs), min(ys), max(xs) - min(xs) + 1, max(ys) - min(ys) + 1)


BOX = ('<label Name="%s" Text="" style="left:%d;top:%d;background:#2E7D32;%s"/>')


def main():
    # 1) 只写宽 + 16/9 → 高必须是 90（160×90）
    img = shot('<window Width="420" Height="300">' +
               (BOX % ('a', 20, 20, 'width:160;aspect-ratio:16/9')) +
               '</window>', os.path.join(ROOT, 'log', '_t_aspect_w.png'))
    bx = bbox(img, RGB)
    ok = bx is not None and abs(bx[2] - 160) <= 3 and abs(bx[3] - 90) <= 3
    check(ok, 'width:160 + aspect-ratio:16/9 → 160×90', bx)

    # 2) 只写高 + 2/1 → 宽必须是 2 倍（240×120）
    img = shot('<window Width="520" Height="320">' +
               (BOX % ('b', 20, 20, 'height:120;aspect-ratio:2/1')) +
               '</window>', os.path.join(ROOT, 'log', '_t_aspect_h.png'))
    bx = bbox(img, RGB)
    ok = bx is not None and abs(bx[2] - 240) <= 3 and abs(bx[3] - 120) <= 3
    check(ok, 'height:120 + aspect-ratio:2/1 → 240×120', bx)

    # 3) 两边都写：显式声明优先，比例不覆盖（120×120）
    img = shot('<window Width="420" Height="300">' +
               (BOX % ('c', 20, 20, 'width:120;height:120;aspect-ratio:16/9')) +
               '</window>', os.path.join(ROOT, 'log', '_t_aspect_both.png'))
    bx = bbox(img, RGB)
    ok = bx is not None and abs(bx[2] - 120) <= 3 and abs(bx[3] - 120) <= 3
    check(ok, '两边都写时以显式声明为准 → 120×120', bx)

    # 4) 不写 aspect-ratio 的对照：同样 width:160 但不是 90 高（默认高度）
    img = shot('<window Width="420" Height="300">' +
               (BOX % ('d', 20, 20, 'width:160;height:40')) +
               '</window>', os.path.join(ROOT, 'log', '_t_aspect_ref.png'))
    bx = bbox(img, RGB)
    ok = bx is not None and abs(bx[2] - 160) <= 3 and abs(bx[3] - 40) <= 3
    check(ok, '对照组（无 aspect-ratio）仍按显式高 → 160×40', bx)

    # 5) 小数比例 1.5 → 160 / 1.5 ≈ 107
    img = shot('<window Width="420" Height="300">' +
               (BOX % ('e', 20, 20, 'width:150;aspect-ratio:1.5')) +
               '</window>', os.path.join(ROOT, 'log', '_t_aspect_15.png'))
    bx = bbox(img, RGB)
    ok = bx is not None and abs(bx[2] - 150) <= 3 and abs(abs(bx[3] - 100) <= 3)
    check(ok, 'width:150 + aspect-ratio:1.5 → 150×100', bx)

    # 6) flex 容器（div → XFlexPanel）里：只写宽 + 16/9 → 160×90
    img = shot('<window Width="460" Height="560">'
               '<div style="height:500;padding:8">'
               '<label Text="" style="width:160;aspect-ratio:16/9;background:#2E7D32"/>'
               '</div></window>', os.path.join(ROOT, 'log', '_t_aspect_flex_w.png'))
    bx = bbox(img, RGB)
    ok = bx is not None and abs(bx[2] - 160) <= 3 and abs(bx[3] - 90) <= 3
    check(ok, 'div 内 width:160 + 16/9 → 160×90（控件层 flex 交叉轴）', bx)

    # 7) flex 容器里只写高 + 16/9 → 比例反推宽 160（CSS：另一边由比例推出来）
    img = shot('<window Width="460" Height="560">'
               '<div style="height:500;padding:8">'
               '<label Text="" style="height:90;aspect-ratio:16/9;background:#2E7D32"/>'
               '</div></window>', os.path.join(ROOT, 'log', '_t_aspect_flex_h.png'))
    bx = bbox(img, RGB)
    ok = bx is not None and abs(bx[2] - 160) <= 3 and abs(bx[3] - 90) <= 3
    check(ok, 'div 内 height:90 + 16/9 → 160×90（另一轴按比例反推，不被 stretch 拉满）', bx)


    print('t_aspect_ratio: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())

