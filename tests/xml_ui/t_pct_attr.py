# -*- coding: utf-8 -*-
"""百分比尺寸：Width="100%"（属性写法）与 style="width:100%"（CSS 写法）必须等价。

HTML 里写 width:100% 就占满父容器；XML 属性写法此前被执行器当成
_wtoi("100%") == 100，也就是 100px —— 一个真 bug：写得越像 CSS，结果越错。
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


def load(xml):
    names = {}
    win = M.guiload_text(xml, context=globals(), names=names)
    return win, names


def rect(win, names, key):
    return win.get_rect(names[key])


BOX = """<XWindow Name="root" Width="440" Height="240">
  <div Name="p" Width="400" Height="200">
    %s
  </div>
</XWindow>"""


def main():
    for label, child, expect in (
        ('属性 Width="100%"', '<label Name="a" Width="100%" Text="fill"/>', (400, 24)),
        ('style width:100%', '<label Name="a" Text="fill" style="width:100%"/>', (400, 24)),
        ('style width+height 100%', '<div Name="a" style="width:100%;height:100%"/>', (400, 200)),
    ):
        win, n = load(BOX % child)
        r = rect(win, n, 'a')
        check(r[2] == expect[0] and r[3] == expect[1], '%s → %dx%d 铺满容器' % (label, expect[0], expect[1]), r)
        try:
            win.close()
        except Exception:
            pass

    # 显式像素优先于默认，50% 走容器换算（控件层容器）
    win, n = load(BOX % '<label Name="a" Width="120" Text="x"/>')
    r = rect(win, n, 'a')
    check(r[2] == 120, '显式像素宽度不被百分比逻辑吃掉', r)
    try:
        win.close()
    except Exception:
        pass

    print('\nt_pct_attr: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    for f in FAILS:
        print('  - ' + f)
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
