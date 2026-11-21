# -*- coding: utf-8 -*-
"""<div> 的 CSS 语义：display:flex 是横排、裸 div 是块级竖排，显式尺寸不被 stretch 拉满。

HTML 侧：<div style="display:flex"> 一行就得到 flex-direction:row（CSS 默认值）；
XML 侧此前只能用私有约定 class="row"，且裸 div 里子项的显式 Width 会被交叉轴 stretch 吃成满宽。
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
    names = {}
    win = M.guiload_text(xml, context=globals(), names=names)
    return win, names


def rect(win, names, key):
    return win.get_rect(names[key])


def case(open_tag, close_tag='</div>'):
    xml = ('<XWindow Name="root" Width="400" Height="200">' + open_tag +
           '<XLabel Name="a" Width="100" Height="40" Text="A"/>'
           '<XLabel Name="b" Width="100" Height="40" Text="B"/>' +
           close_tag + '</XWindow>')
    win, n = load(xml)
    a, b = rect(win, n, 'a'), rect(win, n, 'b')
    try:
        win.close()
    except Exception:
        pass
    return a, b


def main():
    # 1. display:flex → 主轴 row（CSS 默认），gap 生效
    a, b = case('<div style="display:flex;gap:8px">')
    check(a[0] == 0 and b[0] == 108, 'div style="display:flex" 横排且 gap:8 生效', (a, b))
    check(a[1] == 0 and b[1] == 0, '横排两子项同一 y', (a, b))

    # 2. 裸 div 是块级（竖排）
    a, b = case('<div style="gap:8px">')
    check(a[1] == 0 and b[1] == 48, '裸 div 竖排（block 流）且 gap:8 生效', (a, b))

    # 3. 显式 Width 不被交叉轴 stretch 拉满（旧实现会变 384）
    check(a[2] == 100 and b[2] == 100, 'div 里子项显式 Width=100 不被拉满', (a, b))

    # 4. flex-direction:column 显式写法压过 display:flex 的 row
    a, b = case('<div style="display:flex;flex-direction:column;gap:8px">')
    check(b[1] == 48, 'style 里 flex-direction:column 覆盖 display:flex 的默认 row', (a, b))

    # 5. class="row" 老写法仍然可用（兼容）
    a, b = case('<div class="row" style="gap:8px">')
    check(b[0] == 108, 'class="row" 老写法仍横排', (a, b))

    print('\nt_div_css: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    if FAILS:
        for f in FAILS:
            print('  - ' + f)
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
