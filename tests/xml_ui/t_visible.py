# -*- coding: utf-8 -*-
"""HTML 的隐藏语义（`hidden` / `visibility:hidden` / `display:none` / XML `Visible="False"`）
必须真的把东西藏起来。

缺口（第 33 轮审计）：这些键在 XML 侧只被"认名字"，PyMfcLoad 的属性下发里从来没有
消费者 —— 写了 `Visible="False"` 控件照旧在屏幕上。HTML 里这是最基础的可见性开关，
XML 侧完全做不到，只能靠脚本调 setVisible。

断言：同一场景里被隐藏的色块像素不出现；运行期 setVisible 能恢复。
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

BG = (49, 56, 66)
RED = (255, 0, 0)
GREEN = (0, 255, 0)
HIDDEN_AT = (188, 131)   # 隐藏块中心（left:120 top:60 w:120 h:80 + 截图偏移）
VISIBLE_AT = (188, 31)   # 可见块中心（left:120 top:0 → 截图 y≈31）


def check(cond, what, got=None):
    CHECKS[0] += 1
    print(('PASS ' if cond else 'FAIL ') + what + ('' if got is None else '  got=%r' % (got,)))
    if not cond:
        FAILS.append(what)
    return bool(cond)


def near(c, rgb, tol=40):
    return abs(c[0] - rgb[0]) <= tol and abs(c[1] - rgb[1]) <= tol and abs(c[2] - rgb[2]) <= tol


def shot(xml, png):
    path = os.path.join(ROOT, 'log', '_t_visible.xml')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(xml)
    win = M.guiload(path, context={}, names={})
    win.show()
    M.pump(350)
    win.save_png(png)
    return win, Image.open(png).convert('RGB')


def px(img, xy):
    return img.load()[xy]


HID = '<label Name="h" Text="" %s style="left:120;top:60;width:120;height:80;background:#FF0000%s"/>'
VIS = '<label Name="v" Text="" style="left:120;top:0;width:120;height:60;background:#00FF00"/>'


def scene(hidden_attr, hidden_style=''):
    return ('<window Width="400" Height="240">' +
            (HID % (hidden_attr, hidden_style)) + VIS + '</window>')


def main():
    # 0. 基准：都不写隐藏 → 两块都在
    win, img = shot(scene(''), os.path.join(ROOT, 'log', '_t_vis_on.png'))
    check(near(px(img, HIDDEN_AT), RED), '基准：红块可见', px(img, HIDDEN_AT))
    check(near(px(img, VISIBLE_AT), GREEN), '基准：绿块可见', px(img, VISIBLE_AT))
    win.close()

    # 1. XML 属性 Visible="False"
    win, img = shot(scene('Visible="False"'), os.path.join(ROOT, 'log', '_t_vis_attr.png'))
    check(near(px(img, HIDDEN_AT), BG), 'Visible="False" 真的隐藏（露出底色）', px(img, HIDDEN_AT))
    check(near(px(img, VISIBLE_AT), GREEN), '隐藏只作用于声明者（绿块仍在）', px(img, VISIBLE_AT))

    # 2. 运行期恢复
    win.set_visible(1000, True)
    M.pump(300)
    win.save_png(os.path.join(ROOT, 'log', '_t_vis_back.png'))
    img2 = Image.open(os.path.join(ROOT, 'log', '_t_vis_back.png')).convert('RGB')
    check(near(px(img2, HIDDEN_AT), RED), 'setVisible(True) 能把隐藏的控件放出来', px(img2, HIDDEN_AT))
    win.close()

    # 3. style="display:none"
    win, img = shot(scene('', ';display:none'), os.path.join(ROOT, 'log', '_t_vis_none.png'))
    check(near(px(img, HIDDEN_AT), BG), 'style="display:none" 隐藏', px(img, HIDDEN_AT))
    win.close()

    # 4. style="visibility:hidden"
    win, img = shot(scene('', ';visibility:hidden'), os.path.join(ROOT, 'log', '_t_vis_vh.png'))
    hidden_vh = near(px(img, HIDDEN_AT), BG)
    print('      visibility:hidden -> %s' % ('hidden' if hidden_vh else 'still visible'))
    check(hidden_vh, 'style="visibility:hidden" 隐藏', px(img, HIDDEN_AT))
    win.close()

    print('t_visible: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    if FAILS:
        print('failed: ' + '; '.join(FAILS))
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
