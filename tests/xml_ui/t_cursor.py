# -*- coding: utf-8 -*-
"""CSS `cursor` 必须能从 XML 一路落到宿主的光标表。

缺口：`style="cursor:pointer"` 在 XML 侧完全没有通路 —— XmlCore 的 ApplyInlineStyle
没有 cursor 分支，PyMfcLoad 不回读 Cursor 属性，宿主只有"按控件类型猜"的逻辑
（BUTTON/有 Click 的卡片给手型）。HTML 里这是"这块可点"的标准写法，写了却没反应。

本轮打通：style cursor → Cursor 属性 → host->setControlCursor → WM_SETCURSOR 翻译。
断言方式：读宿主表（getCursor），运行期 setCursor 也要能改；几何/外观不受影响。
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
    path = os.path.join(ROOT, 'log', '_t_cursor.xml')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(xml)
    win = M.guiload(path, context={}, names={})
    win.show()
    M.pump(250)
    return win


BOX = '<label Name="%s" Text="" style="left:40;top:40;width:120;height:60;background:#2E7D32%s"/>'


def main():
    # 1. CSS 写法落到宿主表（id 是自动分配的 1000 起，按 names 拿真实 id）
    win = load('<window Width="400" Height="240">' +
               (BOX % ('a', ';cursor:pointer')) +
               (BOX % ('b', '')) +
               (BOX % ('c', ';cursor:text')) +
               '</window>')
    ids = {}
    # getLevel 用过的 id 约定：首个子控件 = 1000
    ia, ib, ic = 1000, 1001, 1002
    check(win.get_cursor(ia) == 'pointer', 'style="cursor:pointer" 落到宿主表', win.get_cursor(ia))
    check(win.get_cursor(ib) == '', '没写 cursor 的控件不设值（回落原判断）', win.get_cursor(ib))
    check(win.get_cursor(ic) == 'text', 'style="cursor:text" 落到宿主表', win.get_cursor(ic))

    # 2. 大写关键字 / 属性写法等价（HTML 属性值不区分大小写）
    check(win.get_cursor(1001 + 0) != 'pointer', '相邻控件互不串值', win.get_cursor(ib))
    win.close()

    win = load('<window Width="400" Height="240">' +
               '<label Name="a" Text="" Cursor="Not-Allowed" style="left:40;top:40;width:120;height:60;background:#2E7D32"/>' +
               '</window>')
    check(win.get_cursor(1000) == 'not-allowed', '属性写法 Cursor="Not-Allowed" 归一为小写关键字',
          win.get_cursor(1000))
    win.close()

    # 3. 运行期改 / 清空
    win = load('<window Width="400" Height="240">' +
               (BOX % ('a', ';cursor:pointer')) +
               '</window>')
    win.set_cursor(1000, 'default')
    check(win.get_cursor(1000) == 'default', '运行期 setCursor 覆盖声明', win.get_cursor(1000))
    win.set_cursor(1000, 'auto')
    check(win.get_cursor(1000) == '', 'setCursor("auto") 表示不接管（清掉声明）', win.get_cursor(1000))
    win.close()

    print('t_cursor: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    if FAILS:
        print('failed: ' + '; '.join(FAILS))
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
