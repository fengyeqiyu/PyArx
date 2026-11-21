# -*- coding: utf-8 -*-
"""同一界面「XML 属性写法」与「HTML/CSS 写法」落位必须完全一致（任务验收第 7 条）。

两份文件在 examples/flex_demo：
  flex_demo.xml      — XStackPanel + Display/Flex/Gap/Justify/Align/Grow
  flex_demo_html.xml — div + style="display:flex;flex-direction;gap;flex:1"、button/input/label
对照 13 个控件矩形，任何一项差 1px 都算退步。
"""
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')

import PyMfcXml as M  # noqa: E402

DEMO = os.path.join(ROOT, 'examples', 'flex_demo')
NAMES = ['page', 'bar', 'tools', 'cards', 'c1', 'c2', 'c3', 'form', 'inp', 'apply',
         'foot', 'ok', 'cancel']

FAILS = []
CHECKS = [0]


def check(cond, what, got=None):
    CHECKS[0] += 1
    print(('PASS ' if cond else 'FAIL ') + what + ('' if got is None else '  got=%r' % (got,)))
    if not cond:
        FAILS.append(what)


def main():
    a = M.xml_layout(os.path.join(DEMO, 'flex_demo.xml'), 704, 404)
    b = M.xml_layout(os.path.join(DEMO, 'flex_demo_html.xml'), 704, 404)
    worst = 0
    for n in NAMES:
        if n not in a or n not in b:
            check(False, '%s 两版都应有' % n, (n in a, n in b))
            continue
        d = max(abs(x - y) for x, y in zip(a[n], b[n]))
        worst = max(worst, d)
        check(d == 0, '%-7s XML=%s HTML=%s' % (n, tuple(a[n]), tuple(b[n])), d)
    check(worst == 0, '两版最大落位偏差 = 0', worst)

    print('\nt_html_parity: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    for f in FAILS:
        print('  - ' + f)
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
