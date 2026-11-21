# -*- coding: utf-8 -*-
"""CSS `pointer-events` 必须能从 XML 一路落到真实窗口命中链。

缺口：`style="pointer-events:none"` 在 XML 侧完全没有通路 —— 写了等于没写。
HTML 里这是"覆盖层/提示层不挡点击"的标准写法，XML 只能整块 Visible=False 拿掉。

本轮打通：style pointer-events → PointerEvents 属性 → host->setControlPointerEvents
→ 控件窗口挂 WS_EX_TRANSPARENT（鼠标从命中链整体消失，点击落到下面兄弟）。
断言方式：读宿主表（getPointerEvents）+ 读窗口真实扩展样式（isMouseTransparent）。
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


def check(cond, what, got=None):
    CHECKS[0] += 1
    print(('PASS ' if cond else 'FAIL ') + what + ('' if got is None else '  got=%r' % (got,)))
    if not cond:
        FAILS.append(what)
    return bool(cond)


def load(xml):
    path = os.path.join(ROOT, 'log', '_t_pointer_events.xml')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(xml)
    win = M.guiload(path, context={}, names={})
    win.show()
    M.pump(250)
    return win


BOX = '<label Name="%s" Text="" style="left:40;top:40;width:120;height:60;background:#2E7D32%s"/>'


def main():
    win = load('<window Width="400" Height="240">' +
               (BOX % ('a', ';pointer-events:none')) +
               (BOX % ('b', '')) +
               (BOX % ('c', ';pointer-events:auto')) +
               '</window>')
    ia, ib, ic = 1000, 1001, 1002
    check(win.get_pointer_events(ia) == 'none', 'style="pointer-events:none" 落到宿主表',
          win.get_pointer_events(ia))
    check(win.get_pointer_events(ib) == 'auto', '没写 pointer-events 视为 auto',
          win.get_pointer_events(ib))
    check(win.get_pointer_events(ic) == 'auto', '显式 auto 不落表（回落正常命中）',
          win.get_pointer_events(ic))

    # 真实窗口样式：只有 none 的那个挂 WS_EX_TRANSPARENT
    check(win.is_mouse_transparent(ia) is True, 'none → 窗口挂 WS_EX_TRANSPARENT（点击穿透）',
          win.is_mouse_transparent(ia))
    check(win.is_mouse_transparent(ib) is False, '未声明 → 不挂透明位',
          win.is_mouse_transparent(ib))
    check(win.is_mouse_transparent(ic) is False, 'auto → 不挂透明位',
          win.is_mouse_transparent(ic))

    # 运行期改：HTML 里常由脚本切换覆盖层是否可点
    win.set_pointer_events(ib, 'none')
    M.pump(60)
    check(win.get_pointer_events(ib) == 'none' and win.is_mouse_transparent(ib) is True,
          '运行期 setPointerEvents(id,"none") 生效', win.is_mouse_transparent(ib))
    win.set_pointer_events(ib, 'auto')
    M.pump(60)
    check(win.get_pointer_events(ib) == 'auto' and win.is_mouse_transparent(ib) is False,
          '运行期 setPointerEvents(id,"auto") 撤回透明位', win.is_mouse_transparent(ib))

    # 外观不受影响：none 的控件照样按背景色画出来（这是"看不见但还在"的对照）
    png = os.path.join(ROOT, 'log', '_t_pe.png')
    win.save_png(png)
    win.close()
    from PIL import Image

    img = Image.open(png).convert('RGB')
    hits = 0
    for y in range(40, 200):
        for x in range(40, 200):
            r, g, b = img.getpixel((x, y))
            if abs(r - 46) < 12 and abs(g - 125) < 12 and abs(b - 50) < 12:
                hits += 1
    check(hits > 300, 'pointer-events:none 的控件仍然正常绘制（只影响命中，不改外观）', hits)

    print('t_pointer_events: PASS=%d FAIL=%d' % (len(CHECKS) and CHECKS[0] - len(FAILS), len(FAILS)))
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
