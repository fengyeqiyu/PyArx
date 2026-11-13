# -*- coding: utf-8 -*-
"""用 PyMfcXml 的 Direct2D 离屏渲染把三级配电出成 PNG，并做同尺寸 1x/2x 两份"""
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M


def three_level():
    nodes = [{'code': 'TR1', 'name': '1号变压器', 'upstream': '',
              'cable': 'YJV22-4×95+1×50', 'loadKw': 0.0}]
    for i in (1, 2):
        nodes.append({'code': 'AT%d' % i, 'name': '%d号柜' % i, 'upstream': 'TR1',
                      'cable': 'YJV-4×35+1×16', 'loadKw': 0.0})
        for j in (1, 2, 3):
            nodes.append({'code': 'AT%d-%d' % (i, j), 'name': '%d号柜%d号回路' % (i, j),
                          'upstream': 'AT%d' % i, 'cable': 'YJV-4×35+1×16', 'loadKw': 0.0})
            for k in (1, 2, 3, 4, 5):
                nodes.append({'code': 'AT%d-%d-%d' % (i, j, k),
                              'name': '%d号柜%d-%d末端' % (i, j, k),
                              'upstream': 'AT%d-%d' % (i, j),
                              'cable': 'BV-3×4', 'loadKw': float(2 + k)})
    return nodes


def main():
    nodes = three_level()
    style = M.powerplan.Style()
    out = r'G:\PyArx\docs'
    for scale in (1, 2):
        path = os.path.join(out, '_pp_xml_d2d_%dx.png' % scale)
        w, h = M.powerplan.renderPng(path, nodes, style, scale)
        print('scale=%d -> %dx%d, %d 字节' % (scale, w, h, os.path.getsize(path)))


if __name__ == '__main__':
    main()
