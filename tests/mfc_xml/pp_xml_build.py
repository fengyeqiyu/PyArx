# -*- coding: utf-8 -*-
"""用 PyMfcXml.powerplan 生成三级配电示例的 XML/JSON/CSV，并落盘"""
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M


def three_level():
    """TR1 → AT1/AT2 → 每柜 3 路 → 每路 5 末端，共 39 节点"""
    nodes = []
    nodes.append({'code': 'TR1', 'name': '1号变压器', 'upstream': '',
                  'cable': 'YJV22-4×95+1×50', 'loadKw': 0.0})
    for i in (1, 2):
        nodes.append({'code': 'AT%d' % i, 'name': '%d号柜' % i, 'upstream': 'TR1',
                      'cable': 'YJV-4×35+1×16', 'loadKw': 0.0})
        for j in (1, 2, 3):
            nodes.append({'code': 'AT%d-%d' % (i, j),
                          'name': '%d号柜%d号回路' % (i, j), 'upstream': 'AT%d' % i,
                          'cable': 'YJV-4×35+1×16', 'loadKw': 0.0})
            for k in (1, 2, 3, 4, 5):
                nodes.append({'code': 'AT%d-%d-%d' % (i, j, k),
                              'name': '%d号柜%d-%d末端' % (i, j, k),
                              'upstream': 'AT%d-%d' % (i, j),
                              'cable': 'BV-3×4', 'loadKw': float(2 + k)})
    return nodes


def main():
    nodes = three_level()
    style = M.powerplan.Style()
    r = M.powerplan.build(nodes, style, '三级配电干线图')
    print('节点 %d，画布 %dx%d，交叉 %d，警告 %d'
          % (len(nodes), r['width'], r['height'], r['crossings'], len(r['warnings'])))
    for w in r['warnings']:
        print('  警告:', w)
    print('boxes=%d links=%d xml=%d 字符 json=%d csv=%d'
          % (len(r['boxes']), len(r['links']), len(r['xml']), len(r['json']), len(r['csv'])))

    out_dir = r'G:\PyArx\docs'
    with open(os.path.join(out_dir, '_pp_xml_three.xml'), 'w', encoding='utf-8') as f:
        f.write(r['xml'])
    with open(os.path.join(out_dir, '_pp_xml_three.json'), 'w', encoding='utf-8') as f:
        f.write(r['json'])
    with open(os.path.join(out_dir, '_pp_xml_three.csv'), 'w', encoding='utf-8-sig', newline='') as f:
        f.write(r['csv'])
    print('已写盘：_pp_xml_three.xml / .json / .csv')

    # CSv 回读校验
    csv_text = open(os.path.join(out_dir, '_pp_xml_three.csv'), encoding='utf-8-sig').read()
    back = M.powerplan.parseCsv(csv_text)
    print('CSV 回读 %d 节点，标题=%r' % (len(back['nodes']), back['title']))


if __name__ == '__main__':
    main()
