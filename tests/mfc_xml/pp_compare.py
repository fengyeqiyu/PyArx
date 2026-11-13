# -*- coding: utf-8 -*-
"""对比 HTML 完整实现：新数据模型（电压/长度/角色/折叠）+ 纵向横向布局 + SVG 出口。

跑三件事：
1. build 纵向 / 横向，打印尺寸、交叉、角色统计、电压统计（与 HTML 口径对照）
2. renderPng 纵向 1x/2x、横向 1x —— Direct2D 离屏看电压角标与电缆标注
3. toSvg（含图框）→ svg.renderPng —— 与 HTML 的"导出 SVG"同一条路
"""
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml

DOCS = r'G:\PyArx\docs'


def three_level():
    """三级配电示例（与 HTML btn3Level 同构）：TR1 → AT1/AT2 → 每柜 3 路 → 每路 5 末端。"""
    nodes = [{'code': 'TR1', 'name': '1号变压器', 'upstream': '',
              'cable': '', 'loadKw': 0, 'voltage': '10kV', 'length': 0}]
    for i in (1, 2):
        nodes.append({'code': 'AT%d' % i, 'name': '%d号配电柜' % i, 'upstream': 'TR1',
                      'cable': 'YJV22-4×95', 'loadKw': 0, 'voltage': '380', 'length': 12.5})
        for j in (1, 2, 3):
            code = 'AT%d-%d' % (i, j)
            nodes.append({'code': code, 'name': '回路%d' % j, 'upstream': 'AT%d' % i,
                          'cable': 'YJV22-4×35', 'loadKw': 8 + j,
                          'voltage': '380', 'length': 6 + j})
            for k in (1, 2, 3, 4, 5):
                nodes.append({'code': '%s-%d' % (code, k), 'name': '',
                              'upstream': code, 'cable': 'BV-3×6',
                              'loadKw': 2 + k,
                              'voltage': '220' if k <= 3 else '380',
                              'length': 3 + k * 2})
    return nodes


def stat(nodes):
    roles, volts = {}, {}
    for n in nodes:
        roles[n['role']] = roles.get(n['role'], 0) + 1
        volts[n['voltage']] = volts.get(n['voltage'], 0) + 1
    return roles, volts


def main():
    nodes = three_level()
    print('节点数 =', len(nodes))

    st = PyMfcXml.powerplan.Style()
    for horiz, tag in ((False, 'v'), (True, 'h')):
        r = PyMfcXml.powerplan.build(nodes, st, '三级配电示例', horiz)
        roles, volts = stat(r['nodes'])
        print('[%s] %dx%d crossings=%d warnings=%s dir=%s' %
              (tag, r['width'], r['height'], r['crossings'], r['warnings'], r['dir']))
        print('    角色', roles, ' 电压', volts)
        print('    xml %d 字符 / json %d / csv %d / svg %d' %
              (len(r['xml']), len(r['json']), len(r['csv']), len(r['svg'])))
        with open(os.path.join(DOCS, '_pp_new_%s.svg' % tag), 'w', encoding='utf-8') as f:
            f.write(r['svg'])
        png = os.path.join(DOCS, '_pp_new_%s_1x.png' % tag)
        w, h = PyMfcXml.powerplan.renderPng(png, nodes, st, 1, horiz)
        print('    renderPng 1x ->', w, 'x', h, os.path.getsize(png), 'B')

    # 2x 纵向，看字形与角标
    png2 = os.path.join(DOCS, '_pp_new_v_2x.png')
    w, h = PyMfcXml.powerplan.renderPng(png2, nodes, st, 2, False)
    print('renderPng 2x ->', w, 'x', h, os.path.getsize(png2), 'B')

    # SVG 用 nanosvg 解析后回绘（HTML 导出 SVG 的那条路）
    src = os.path.join(DOCS, '_pp_new_v.svg')
    dst = os.path.join(DOCS, '_pp_new_v_fromsvg.png')
    info = PyMfcXml.svg.parse(src)
    print('svg.parse:', int(info['width']), 'x', int(info['height']),
          'shapes=%d paths=%d warnings=%d' % (info['shapes'], info['paths'], len(info['warnings'])))
    print('svg.renderPng ->', PyMfcXml.svg.renderPng(src, dst, 1, '#FFFFFF'),
          os.path.getsize(dst), 'B')


if __name__ == '__main__':
    main()
