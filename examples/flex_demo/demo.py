# -*- coding: utf-8 -*-
"""flex 布局示例：加载 flex_demo.xml，打印每个控件落位，并截一张图。

用法：python demo.py [--shot]
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')

import PyMfcXml as M


def main():
    xml_path = os.path.join(HERE, 'flex_demo.xml')
    names = {}
    win = M.guiload(xml_path, context=globals(), names=names)
    win.show()
    M.pump(400)

    rects = M.xml_layout(xml_path, 704, 404)
    for name in ('bar', 'tools', 'cards', 'c1', 'c2', 'c3', 'form', 'inp', 'apply',
                 'foot', 'ok', 'cancel'):
        if name in rects:
            print('%-8s x=%-4d y=%-4d w=%-4d h=%d' % ((name,) + tuple(rects[name])))

    shot = os.path.join(HERE, 'flex_demo.png')
    win.save_png(shot)
    print('shot', shot)
    M.pump(100)
    return 0


if __name__ == '__main__':
    sys.exit(main())
