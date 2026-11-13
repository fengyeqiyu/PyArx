# -*- coding: utf-8 -*-
"""校验 toXaml 现在的输出：一张 <PowerCanvas Data="...JSON..."/>。
以前这里输出 38 个 <Curve> 标签，Viewer --check 全报"未知标签"。
"""
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXaml as M

sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
from pp_canvas_window import three_level


def main():
    nodes = three_level()
    r = M.powerplan.build(nodes, M.powerplan.Style(), '三级配电干线图')
    xaml = r['xaml']
    path = r'G:\PyArx\docs\_pp_xaml_three.xaml'
    with open(path, 'w', encoding='utf-8') as f:
        f.write(xaml)

    print('xaml 字符数 %d' % len(xaml))
    print('含 PowerCanvas =', '<PowerCanvas' in xaml)
    print('含 Curve       =', '<Curve' in xaml)
    print('Data 属性长度  =', xaml.split('Data="', 1)[1].rsplit('"', 1)[0].__len__())

    # 解析回来：确认标签被引擎接受、属性值里的 JSON 没被 XML 转义搞坏
    doc = M.xaml.load(path)
    print('Document.tags =', doc.tags if hasattr(doc, 'tags') else '-')
    print('names =', doc.names)
    print('XAML 回读 OK')


if __name__ == '__main__':
    main()
