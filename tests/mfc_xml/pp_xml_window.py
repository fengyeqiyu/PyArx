# -*- coding: utf-8 -*-
"""端到端：从 XML 文档开窗显示配电图（不写一行布局代码）。

文档由 powerplan.build(...)['xml'] 生成，内容是
  <XWindow ...><XDwgCanvas Name="Plan" Data="{...JSON...}"/></XWindow>
"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M

sys.path.insert(0, r'G:\PyArx\tests\mfc_xml')
from pp_canvas_window import three_level


def main():
    nodes = three_level()
    r = M.powerplan.build(nodes, M.powerplan.Style(), '三级配电干线图')
    path = r'G:\PyArx\docs\_pp_xml_three.xml'
    with open(path, 'w', encoding='utf-8') as f:
        f.write(r['xml'])
    print('XML 长度 %d' % len(r['xml']))

    win = M.guiload(path)
    print('窗口已创建，names=%r' % (win.names() if hasattr(win, 'names') else '-'))
    win.show()
    time.sleep(0.5)
    win.save_png(r'G:\PyArx\docs\_pp_xml_window.png')
    print('已抓 _pp_xml_window.png')
    time.sleep(0.5)
    win.close()


if __name__ == '__main__':
    main()
