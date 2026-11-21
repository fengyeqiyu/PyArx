# -*- coding: utf-8 -*-
"""同一界面的 XML 写法版 与 HTML 写法版 对照：
   打印同名控件的落位差，并各截一张图做像素级 diff。

用法：python compare.py
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')

import PyMfcXml as M  # noqa: E402

NAMES = ['page', 'bar', 'tools', 'cards', 'c1', 'c2', 'c3', 'form', 'inp', 'apply',
         'foot', 'ok', 'cancel']


def layout(path):
    return M.xml_layout(path, 704, 404)


def main():
    a = layout(os.path.join(HERE, 'flex_demo.xml'))
    b = layout(os.path.join(HERE, 'flex_demo_html.xml'))
    worst = 0
    for n in NAMES:
        if n not in a or n not in b:
            print('%-8s 缺（xml=%s html=%s）' % (n, n in a, n in b))
            continue
        d = [abs(x - y) for x, y in zip(a[n], b[n])]
        worst = max(worst, max(d))
        print('%-8s xml=%s html=%s  Δ=%s' % (n, tuple(a[n]), tuple(b[n]), tuple(d)))
    print('最大偏差 =', worst)

    # 截图对照
    for tag, path in (('xml', 'flex_demo.xml'), ('html', 'flex_demo_html.xml')):
        names = {}
        win = M.guiload(os.path.join(HERE, path), context=globals(), names=names)
        win.show()
        M.pump(400)
        shot = os.path.join(HERE, 'flex_demo_%s.png' % tag)
        win.save_png(shot)
        print('shot', tag, shot)
        M.pump(100)
        try:
            win.close()
        except Exception:
            pass
    return 0


if __name__ == '__main__':
    sys.exit(main())
