# -*- coding: utf-8 -*-
"""容器内「绝对定位子项」的几何必须相对容器内框落地。

HTML：`<div style="left:40;top:40;width:180;height:100"><label style="left:10;top:10;
width:160;height:32"></label></div>` —— 子元素相对容器定位，不参与外层文档流。
早前这类子项（写了 left/top）被 flex 容器当成 free 元素，只排了它的子树，
自身 preview 恒为 0×0，建出来的窗口尺寸为 0（文字根本看不见）。
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


def check(cond, what, got=None):
    CHECKS[0] += 1
    print(('PASS ' if cond else 'FAIL ') + what + ('' if got is None else '  got=%r' % (got,)))
    if not cond:
        FAILS.append(what)
    return bool(cond)


def shot(xml, png):
    path = os.path.join(ROOT, 'log', '_t_abs_in_div.xml')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(xml)
    win = M.guiload(path, context={}, names={})
    win.show()
    M.pump(400)
    win.save_png(png)
    win.close()
    return Image.open(png).convert('RGB')


def bbox(img, rgb, tol=30):
    px = img.load()
    x0 = y0 = 10 ** 9
    x1 = y1 = -1
    for y in range(img.height):
        for x in range(img.width):
            r, g, b = px[x, y]
            if abs(r - rgb[0]) <= tol and abs(g - rgb[1]) <= tol and abs(b - rgb[2]) <= tol:
                x0 = min(x0, x)
                y0 = min(y0, y)
                x1 = max(x1, x)
                y1 = max(y1, y)
    if x1 < 0:
        return None
    return (x0, y0, x1 + 1, y1 + 1)


CARD = ('<div Name="card" style="left:40;top:40;width:180;height:100;background:#2E7D32">%s</div>')


def main():
    # 截图坐标相对客户区有固定偏移，先量容器自身的位置作为基准
    img = shot('<window Width="460" Height="260">' + (CARD % '') + '</window>',
               os.path.join(ROOT, 'log', '_t_abs_card.png'))
    card = bbox(img, (46, 125, 50))
    check(card is not None and card[2] - card[0] == 180 and card[3] - card[1] == 100,
          '容器背板 180x100', card)

    # 1. 绝对定位：left:10 top:10 width:160 height:32 → 相对容器内框落地
    img = shot('<window Width="460" Height="260">' +
               (CARD % '<label Name="t" Text="" style="left:10;top:10;width:160;height:32;background:#00FF00"/>') +
               '</window>', os.path.join(ROOT, 'log', '_t_abs_left.png'))
    box = bbox(img, (0, 255, 0))
    dx = box[0] - card[0] if box else None
    dy = box[1] - card[1] if box else None
    check(box is not None and box[2] - box[0] == 160 and box[3] - box[1] == 32,
          '容器内绝对定位子项尺寸 160x32（不再退化成 0×0）', box)
    check(dx == 10 and dy == 10, '子项 left/top 相对容器内框（偏移 10,10）', (dx, dy))

    # 2. 百分比宽度按容器内容宽折算：80% × 180 = 144
    img = shot('<window Width="460" Height="260">' +
               (CARD % '<label Name="t" Text="" style="left:10;top:10;width:80%;height:32;background:#00FF00"/>') +
               '</window>', os.path.join(ROOT, 'log', '_t_abs_pct.png'))
    box2 = bbox(img, (0, 255, 0))
    check(box2 is not None and abs((box2[2] - box2[0]) - 144) <= 2,
          '容器内 width:80% 折算成 144px', box2)

    # 3. 只写 right/bottom：贴容器右下角
    img = shot('<window Width="460" Height="260">' +
               (CARD % '<label Name="t" Text="" style="right:10;bottom:10;width:60;height:20;background:#00FF00"/>') +
               '</window>', os.path.join(ROOT, 'log', '_t_abs_right.png'))
    box3 = bbox(img, (0, 255, 0))
    rx = (card[2] - 10) - box3[2] if box3 else None
    ry = (card[3] - 10) - box3[3] if box3 else None
    check(box3 is not None and rx == 0 and ry == 0, '只写 right/bottom 时贴右下角', (rx, ry))

    print('t_abs_in_div: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    if FAILS:
        print('failed: ' + '; '.join(FAILS))
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
