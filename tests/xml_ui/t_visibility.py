# -*- coding: utf-8 -*-
"""CSS `visibility:hidden` 必须"保留占位、只是不画"（与 display:none 区分）。

缺口（44 轮审计）：`style="visibility:hidden"` 只被 Put 成 Visibility 属性，没有任何消费者 ——
写了等于没写，控件照常显示；而 XML 侧唯一的隐藏手段 Visible="False" 语义又等于
"不占位"。HTML 里"占位但不可见"（骨架屏、对齐占位）在 XML 完全无法表达。

断言方式：两个同高 label 竖排，首个 visibility:hidden 后第二个的 y 不变（占位仍在），
而首个的背景色包围盒必须为空（没画）。
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
GREEN = (0x2E, 0x7D, 0x32)
BLUE = (0x15, 0x65, 0xC0)


def check(cond, what, got=None):
    CHECKS[0] += 1
    print(('PASS ' if cond else 'FAIL ') + what + ('' if got is None else '  got=%r' % (got,)))
    if not cond:
        FAILS.append(what)
    return bool(cond)


def shot(xml, png):
    path = os.path.join(ROOT, 'log', '_t_vis.xml')
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


def scene(css):
    return ('<window Width="300" Height="260">'
            '<div style="height:220;padding:8">'
            '<label Text="" style="width:80;height:40;background:#2E7D32;' + css + '"/>'
            '<label Text="" style="width:120;height:40;background:#1565C0"/>'
            '</div></window>')


def main():
    # 1) visibility:hidden → 首块不画（包围盒为空）
    img = shot(scene('visibility:hidden'), os.path.join(ROOT, 'log', '_t_vis_hidden.png'))
    g = bbox(img, GREEN)
    check(g is None, 'visibility:hidden 的元素不绘制', g)
    b_hi = bbox(img, BLUE)
    check(b_hi is not None, 'visibility:hidden 时后面的兄弟仍然显示', b_hi)

    # 2) 对照组 visibility:visible → 首块照常画，兄弟 y 与上面一致（占位没变）
    img2 = shot(scene('visibility:visible'), os.path.join(ROOT, 'log', '_t_vis_visible.png'))
    g2 = bbox(img2, GREEN)
    check(g2 is not None, 'visibility:visible 正常显示', g2)
    b_lo = bbox(img2, BLUE)
    check(b_lo is not None and b_hi is not None and abs(b_lo[1] - b_hi[1]) <= 2,
          'visibility:hidden 保留占位（兄弟 y 与 visible 时一致）', (b_hi, b_lo))
    check(g2 is not None and abs(g2[3] - 40) <= 2, 'visible 时首块高度仍是声明的 40', g2)

    # 3) 属性写法也可以（非 style）：Visibility="hidden"
    xml3 = ('<window Width="300" Height="260">'
            '<label Name="x" Text="" Visibility="hidden" '
            'style="left:20;top:20;width:80;height:40;background:#2E7D32"/>'
            '</window>')
    img3 = shot(xml3, os.path.join(ROOT, 'log', '_t_vis_attr.png'))
    g3 = bbox(img3, GREEN)
    check(g3 is None, 'Visibility="hidden" 属性同样不绘制', g3)

    print('t_visibility: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
