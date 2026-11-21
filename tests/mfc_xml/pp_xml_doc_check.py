# -*- coding: utf-8 -*-
"""校验 toXml 现在的输出：一张 <XDwgCanvas Data="...JSON..."/>。
以前这里输出 38 个 <Curve> 标签，Viewer --check 全报"未知标签"。
"""
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M

sys.path.insert(0, r'G:\PyArx\tests\mfc_xml')
from pp_canvas_window import three_level


def main():
    nodes = three_level()
    r = M.powerplan.build(nodes, M.powerplan.Style(), '三级配电干线图')
    xml = r['xml']
    path = r'G:\PyArx\docs\_pp_xml_three.xml'
    with open(path, 'w', encoding='utf-8') as f:
        f.write(xml)

    print('xml 字符数 %d' % len(xml))
    print('含 DwgCanvas =', '<XDwgCanvas' in xml)
    print('含 Curve       =', '<Curve' in xml)
    print('Data 属性长度  =', xml.split('Data="', 1)[1].rsplit('"', 1)[0].__len__())

    # 解析回来：确认标签被引擎接受、属性值里的 JSON 没被 XML 转义搞坏
    doc = M.xml.load(path)
    print('Document.tags =', doc.tags if hasattr(doc, 'tags') else '-')
    print('names =', doc.names)
    print('XML 回读 OK')


if __name__ == '__main__':
    main()
