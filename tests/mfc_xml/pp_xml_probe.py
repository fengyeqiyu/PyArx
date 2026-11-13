# -*- coding: utf-8 -*-
"""验证 XML（guiload）能否 1:1 画出 HTML 的圆角/半透明/分组间距。"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M

XML = r'G:\PyArx\docs\_ref\probe_appearance.xml'
OUT = r'G:\PyArx\docs\_pp_xml_probe.png'


def main():
    names = {}
    w = M.guiload(XML, target=None, context=None, names=names, first_id=2000)
    print('窗口', type(w).__name__, 'x:Name -> id:', len(names))
    for k in ('docTitle', 'btnSample', 'btn3Level', 'btnEdit', 'dirSel', 'search', 'statusText', 'mini'):
        print('  ', k, names.get(k))
    w.show()
    time.sleep(0.6)
    M.pump(40)
    w.savePng(OUT)
    print('截图', OUT, os.path.getsize(OUT), 'B')
    # 改属性能否即时重排：把主按钮换成 HTML hover 色
    if names.get('btnSample'):
        w.setXmlValue('btnSample', 'BackColor', '#F1F5F9')
        M.pump(30)
        w.savePng(OUT.replace('.png', '_hover.png'))
        print('hover 版', os.path.getsize(OUT.replace('.png', '_hover.png')), 'B')


if __name__ == '__main__':
    main()
