# -*- coding: utf-8 -*-
"""把 SVG 直接写进 XML 属性：<XDwgCanvas Data="&lt;svg ...&gt;"/>。

用户的要求是"在 xml 里面加入 svg 矢量化图"，路径必须成立：
XML 文档里声明一块画布，属性值就是 SVG 全文，运行时自己解析并绘制。
"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M

SVG = r'G:\PyArx\docs\_pyarx_qml_powerplan.svg'
XML = r'G:\PyArx\docs\_pp_xml_svg.xml'


def esc(text):
    return (text.replace('&', '&amp;').replace('<', '&lt;')
                .replace('>', '&gt;').replace('"', '&quot;'))


def main():
    with open(SVG, 'r', encoding='utf-8') as f:
        svg = f.read()

    doc = ('<?xml version="1.0" encoding="utf-8"?>\r\n'
           '<XWindow Name="SvgDoc" Width="860" Height="620" Title="SVG 在 XML 里" '
           'Background="#FFFAFCFF">\r\n'
           '  <XDwgCanvas Name="Board" Left="0" Top="0" Width="860" Height="620" Data="'
           + esc(svg) + '"/>\r\n'
           '</XWindow>\r\n')
    with open(XML, 'w', encoding='utf-8') as f:
        f.write(doc)
    print('XML 字符数 %d（其中 SVG %d）' % (len(doc), len(svg)))

    win = M.guiload(XML)
    win.show()
    time.sleep(0.4)
    win.save_png(r'G:\PyArx\docs\_pp_xml_svg_window.png')
    print('已抓 _pp_xml_svg_window.png')
    time.sleep(0.3)
    win.close()


if __name__ == '__main__':
    main()
