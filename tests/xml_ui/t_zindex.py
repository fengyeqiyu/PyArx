# -*- coding: utf-8 -*-
"""CSS `z-index` 必须能生效（HTML 标准写法 → 控件层 ZIndex/Level）。

缺口：控件层早就支持属性 `ZIndex`/`Level`（PyMfcHost::setControlLevel + applyZOrder），
但 `style="z-index:5"` 在 ApplyInlineStyle 里没有分支 —— HTML 写对了却静默不生效，
XML 侧只剩 WPF 味的 ZIndex="5"。本轮把 CSS 写法翻译成同一份属性。

判定：两个同尺寸色块完全重叠（红先写、蓝后写），改 z-index 必须改变谁在上层，
几何位置一律不动。截图坐标相对客户区有固定偏移（x≈+8, y≈+31），
所以色块 left:120/top:60 → 截图里约 (128,91)-(248,171)。
"""
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')

import PyMfcXml as M  # noqa: E402
from PIL import Image  # noqa: E402

FAILS = []
CHECKS = [0]

RED = '#FF0000'
BLUE = '#0000FF'
BG = (49, 56, 66)      # 窗口默认深色底
CENTER = (188, 131)    # 两块重叠区域中心
RED_TL = (135, 100)    # 红块内部左上角
RED_BR = (243, 165)    # 红块内部右下角
OUTSIDE = (100, 90)    # 色块左侧、窗口背景


def check(cond, what, got=None):
    CHECKS[0] += 1
    print(('PASS ' if cond else 'FAIL ') + what + ('' if got is None else '  got=%r' % (got,)))
    if not cond:
        FAILS.append(what)
    return bool(cond)


def shot(xml, png):
    path = os.path.join(ROOT, 'log', '_t_zindex.xml')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(xml)
    win = M.guiload(path, context={}, names={})
    win.show()
    M.pump(400)
    win.save_png(png)
    win.close()
    return Image.open(png).convert('RGB')


def px(img, xy):
    return img.load()[xy]


def near(c, rgb, tol=40):
    return abs(c[0] - rgb[0]) <= tol and abs(c[1] - rgb[1]) <= tol and abs(c[2] - rgb[2]) <= tol


BOX = '<label Name="%s" Text="" style="left:120;top:60;width:120;height:80;background:%s%s"/>'


def scene(a_extra='', b_extra=''):
    return ('<window Width="400" Height="220">' +
            (BOX % ('a', RED, a_extra)) +
            (BOX % ('b', BLUE, b_extra)) +
            '</window>')


def main():
    # 基准 1：色块外仍是窗口底色（确认坐标基准正确）
    img = shot(scene(), os.path.join(ROOT, 'log', '_t_z_default.png'))
    check(near(px(img, OUTSIDE), BG), '色块左侧为窗口底色（坐标基准）', px(img, OUTSIDE))

    # 基准 2：默认层叠 = HTML 的「后写的在上」（无 z-index 时不额外干预）
    check(near(px(img, CENTER), (0, 0, 255)), '默认层叠：后写的蓝块在上（对齐 HTML）', px(img, CENTER))

    # 1. CSS 写法：给"先写、本该在下"的红块 z-index:5 就必须翻上来
    img = shot(scene(a_extra=';z-index:5'), os.path.join(ROOT, 'log', '_t_z_css.png'))
    check(near(px(img, CENTER), (255, 0, 0)), 'style="z-index:5" 把红块翻到上层', px(img, CENTER))

    # 2. 负值：给"后写、本该在上"的蓝块 z-index:-5 必须沉下去
    img = shot(scene(b_extra=';z-index:-5'), os.path.join(ROOT, 'log', '_t_z_neg.png'))
    check(near(px(img, CENTER), (255, 0, 0)), 'style="z-index:-5" 把蓝块沉到下层', px(img, CENTER))

    # 3. 属性写法 ZIndex="5" 与 CSS 写法等效（老写法不回归）
    img = shot('<window Width="400" Height="220">' +
               ('<label Name="a" Text="" ZIndex="5" style="left:120;top:60;width:120;height:80;background:%s"/>' % RED) +
               ('<label Name="b" Text="" style="left:120;top:60;width:120;height:80;background:%s"/>' % BLUE) +
               '</window>', os.path.join(ROOT, 'log', '_t_z_attr.png'))
    check(near(px(img, CENTER), (255, 0, 0)), '属性 ZIndex="5" 与 CSS 写法等效', px(img, CENTER))

    # 4. z-index 不改几何：红块左上/右下仍在自己的盒子里
    img = shot(scene(a_extra=';z-index:5'), os.path.join(ROOT, 'log', '_t_z_geom.png'))
    check(near(px(img, RED_TL), (255, 0, 0)), '红块左上角几何未被改写', px(img, RED_TL))
    check(near(px(img, RED_BR), (255, 0, 0)), '红块右下角几何未被改写', px(img, RED_BR))

    print('t_zindex: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    if FAILS:
        print('failed: ' + '; '.join(FAILS))
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
