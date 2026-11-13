# -*- coding: utf-8 -*-
"""端到端：从 XAML 文档开窗显示配电图（不写一行布局代码）。

文档由 powerplan.build(...)['xaml'] 生成，内容是
  <XWindow ...><XPowerCanvas Name="Plan" Data="{...JSON...}"/></XWindow>
"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXaml as M

sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
from pp_canvas_window import three_level


def main():
    nodes = three_level()
    r = M.powerplan.build(nodes, M.powerplan.Style(), '三级配电干线图')
    path = r'G:\PyArx\docs\_pp_xaml_three.xaml'
    with open(path, 'w', encoding='utf-8') as f:
        f.write(r['xaml'])
    print('XAML 长度 %d' % len(r['xaml']))

    win = M.guiload(path)
    print('窗口已创建，names=%r' % (win.names() if hasattr(win, 'names') else '-'))
    win.show()
    time.sleep(0.5)
    win.savePng(r'G:\PyArx\docs\_pp_xaml_window.png')
    print('已抓 _pp_xaml_window.png')
    time.sleep(0.5)
    win.close()


if __name__ == '__main__':
    main()
