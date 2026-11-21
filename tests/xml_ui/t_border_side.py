# -*- coding: utf-8 -*-
"""CSS 单边边框（`border-bottom:1px solid #333` 这类分割线）必须能画出来。

缺口（第 34 轮审计）：外观层只有"整圈边框"（BorderThickness + BorderColor + BorderStyle），
HTML 里最常见的 `border-bottom:1px solid #xxx` 在 XML 侧完全做不到 —— 只能整圈框住盒子。

本轮落地：XmlAppearance 增 borderSideW[4] / borderSideColor[4] / borderSideStyle[4]
（0=上 1=右 2=下 3=左；-1=未声明沿用整圈，0=显式无边），
解析 border-<side> 简写与 -width/-color/-style；绘制在背板之后逐边落像素。
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

BORDER = (0x33, 0x33, 0x33)
BOX_BG = (0x2E, 0x7D, 0x32)


def check(cond, what, got=None):
    CHECKS[0] += 1
    print(('PASS ' if cond else 'FAIL ') + what + ('' if got is None else '  got=%r' % (got,)))
    if not cond:
        FAILS.append(what)
    return bool(cond)


def shot(xml, png):
    path = os.path.join(ROOT, 'log', '_t_border_side.xml')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(xml)
    win = M.guiload(path, context={}, names={})
    win.show()
    M.pump(350)
    win.save_png(png)
    win.close()
    return Image.open(png).convert('RGB')


def px(img, xy):
    return img.load()[xy]


def near(c, rgb, tol=26):
    return abs(c[0] - rgb[0]) <= tol and abs(c[1] - rgb[1]) <= tol and abs(c[2] - rgb[2]) <= tol


BOX = '<label Name="box" Text="" style="left:120;top:60;width:160;height:100;background:#2E7D32;%s"/>'
# 截图固定偏移 x≈+8, y≈+31：盒子客户区 ≈ (128,91)-(288,191)
IN_TOP = (200, 92)
IN_BOT = (200, 190)
IN_LEFT = (129, 140)
IN_RIGHT = (287, 140)
INSIDE = (200, 140)


def main():
    # 1. border-bottom：下沿一条线，另三边保持底色
    img = shot('<window Width="420" Height="260">' +
               (BOX % 'border-bottom:2px solid #333333') + '</window>',
               os.path.join(ROOT, 'log', '_t_bs_bottom.png'))
    check(near(px(img, IN_BOT), BORDER), 'border-bottom 画出下沿线', px(img, IN_BOT))
    check(near(px(img, IN_TOP), BOX_BG), '上沿仍是底色（没有被整圈框住）', px(img, IN_TOP))
    check(near(px(img, INSIDE), BOX_BG), '盒子内部仍是底色', px(img, INSIDE))

    # 2. border-top：只画上沿
    img = shot('<window Width="420" Height="260">' +
               (BOX % 'border-top:2px solid #333333') + '</window>',
               os.path.join(ROOT, 'log', '_t_bs_top.png'))
    check(near(px(img, IN_TOP), BORDER), 'border-top 画出上沿线', px(img, IN_TOP))
    check(near(px(img, IN_BOT), BOX_BG), '下沿仍是底色', px(img, IN_BOT))

    # 3. border-left：只画左沿
    img = shot('<window Width="420" Height="260">' +
               (BOX % 'border-left:2px solid #333333') + '</window>',
               os.path.join(ROOT, 'log', '_t_bs_left.png'))
    check(near(px(img, IN_LEFT), BORDER), 'border-left 画出左沿线', px(img, IN_LEFT))
    check(near(px(img, IN_RIGHT), BOX_BG), '右沿仍是底色', px(img, IN_RIGHT))

    # 4. 单边 + 不同的边宽/边色：下沿 3px 蓝，其余无边
    img = shot('<window Width="420" Height="260">' +
               (BOX % 'border-bottom:3px solid #0000FF') + '</window>',
               os.path.join(ROOT, 'log', '_t_bs_color.png'))
    check(near(px(img, IN_BOT), (0, 0, 255)), '单边颜色独立生效（蓝）', px(img, IN_BOT))

    # 5. 显式 border-bottom:0 — 下沿不应出现线（与整圈边框混写时单边优先）
    img = shot('<window Width="420" Height="260">' +
               (BOX % 'border:2px solid #333333;border-bottom:0') + '</window>',
               os.path.join(ROOT, 'log', '_t_bs_override.png'))
    check(near(px(img, IN_TOP), BORDER), '整圈边框仍在上沿（border:2px 保留）', px(img, IN_TOP))
    check(near(px(img, IN_BOT), BOX_BG), 'border-bottom:0 覆盖整圈的下沿', px(img, IN_BOT))

    print('t_border_side: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    if FAILS:
        print('failed: ' + '; '.join(FAILS))
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
