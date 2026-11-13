# -*- coding: utf-8 -*-
"""三级配电逐级灌入：一级 20 → 二级每级 ×10 → 三级每级 ×40（合计 8220 行）。
每一级单独计时并出画布截图，用来看"一级一级加上去"时布局/画布的表现。"""
import os, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pp_app
import PyMfcXaml as M

N1, M2, K3 = 20, 10, 40
HDR = ''


def rows_l1():
    out = [pp_app.COLS[:pp_app.BOOK_N]]
    for i in range(N1):
        out.append(['', 'WL-L1-%02d' % (i + 1), 'TR1-%02d' % (i + 1), 'YJV22-4×95',
                    '10kV 中压', '1', str(2 + i % 5), '0', '2', str(4 + i % 5), '0'])
    return out


def rows_l2():
    out = []
    for i in range(N1):
        for j in range(M2):
            out.append(['TR1-%02d' % (i + 1), 'WL-L2-%02d-%02d' % (i + 1, j + 1),
                        'AT2-%02d-%02d' % (i + 1, j + 1), 'YJV22-4×35',
                        '380V 三相', '1', str(6 + j % 6), '2', '2', str(10 + j % 6), str(3 + j % 4)])
    return out


def rows_l3():
    out = []
    for i in range(N1):
        for j in range(M2):
            for k in range(K3):
                out.append(['AT2-%02d-%02d' % (i + 1, j + 1),
                            'WL-L3-%02d-%02d-%02d' % (i + 1, j + 1, k + 1),
                            'AT3-%02d-%02d-%02d' % (i + 1, j + 1, k + 1),
                            'BV-3×4' if k % 2 else 'YJV22-4×25',
                            '220V 单相' if k % 2 else '380V 三相',
                            '1', str(4 + k % 8), '2', '2', str(8 + k % 8), str(2 + k % 3)])
    return out


def csv_text(rows):
    parts = [HDR]
    for r in rows:
        parts.append(','.join(r))
    return '\r\n'.join(parts) + '\r\n'


app = pp_app.App()
app.win.show()
M.pump(80)
app.showEditor()
M.pump(150)

stages = [('一级 %d 行' % N1, rows_l1()),
          ('二级 +%d 行' % (N1 * M2), rows_l2()),
          ('三级 +%d 行' % (N1 * M2 * K3), rows_l3())]
allrows = []
for name, rows in stages:
    allrows += rows
    text = csv_text(allrows)
    t0 = time.perf_counter()
    rs = pp_app.App.parseCsvText(text)
    nodes = app.nodesFromRows(rs)
    app.loadNodes(nodes, name)
    M.pump(300)
    dt = (time.perf_counter() - t0) * 1000
    print('%s：累计 %d 节点，loadNodes %.1f ms，画布 %sx%s' %
          (name, len(nodes), dt, app.cvW, app.cvH), flush=True)
    tag = {'一级 20 行': 'lv1', '二级 +200 行': 'lv2'}.get(name, 'lv3')
    app.win.savePng(r'G:\PyArx\docs\_tree3_%s.png' % tag)
    M.pump(200)

print('done', flush=True)
M.pump(4000)
