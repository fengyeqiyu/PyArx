# -*- coding: utf-8 -*-
"""三级配电 20/200/8000 结构：纯引擎分段计时（不建窗口）。"""
import os, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import engine, pp_app

N1, M2, K3 = 20, 10, 40
rows = [pp_app.COLS[:pp_app.BOOK_N]]
for i in range(N1):
    rows.append(['', 'WL-L1-%02d' % (i + 1), 'TR1-%02d' % (i + 1), 'YJV22-4×95', '10kV 中压', '1', '3', '0', '2', '5', '0'])
for i in range(N1):
    for j in range(M2):
        rows.append(['TR1-%02d' % (i + 1), 'WL-L2-%02d-%02d' % (i + 1, j + 1), 'AT2-%02d-%02d' % (i + 1, j + 1),
                     'YJV22-4×35', '380V 三相', '1', '8', '2', '2', '12', '4'])
for i in range(N1):
    for j in range(M2):
        for k in range(K3):
            rows.append(['AT2-%02d-%02d' % (i + 1, j + 1), 'WL-L3-%02d-%02d-%02d' % (i + 1, j + 1, k + 1),
                         'AT3-%02d-%02d-%02d' % (i + 1, j + 1, k + 1), 'BV-3×4', '220V 单相', '1', '6', '2', '2', '10', '3'])

text = '\r\n'.join(','.join(r) for r in rows) + '\r\n'
t0 = time.perf_counter(); rs = pp_app.App.parseCsvText(text); t1 = time.perf_counter()
nodes = pp_app.App.nodesFromRows(rs); t2 = time.perf_counter()
print('行 %d 解析 %.1f ms 转节点 %.1f ms' % (len(rs) - 1, (t1 - t0) * 1000, (t2 - t1) * 1000), flush=True)
ENG = engine.PowerPlanEngine()
t3 = time.perf_counter(); lay = ENG.layout(nodes, False); t4 = time.perf_counter()
print('engine.layout %.1f ms (%d 节点 %d 连线)' % ((t4 - t3) * 1000, len(lay['nodes']), len(lay['links'])), flush=True)
t5 = time.perf_counter(); cc = ENG.countCrossings(lay['links']); t6 = time.perf_counter()
print('countCrossings %.1f ms -> %d' % ((t6 - t5) * 1000, cc), flush=True)
t7 = time.perf_counter(); sc = ENG.scene(nodes, False, 'tree3'); t8 = time.perf_counter()
print('engine.scene %.1f ms' % ((t8 - t7) * 1000), flush=True)
