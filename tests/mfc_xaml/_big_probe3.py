# -*- coding: utf-8 -*-
"""引擎侧分段计时（不建窗口）：12000 行 CSV → 节点 → layout / scene / countCrossings。"""
import csv, io, os, sys, time, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import engine
import pp_app

def gen(count):
    buf = io.StringIO()
    wr = csv.writer(buf, lineterminator='\r\n')
    wr.writerow(pp_app.COLS[:pp_app.BOOK_N])
    for i in range(count):
        wr.writerow(['TR1', 'WL-%05d' % i, 'AT-%05d' % i, 'YJV22-4×95', '380V 三相',
                     1, '%.1f' % (10 + i % 20), '2', '2', '%d' % (13 + i % 20), '3.0'])
    return buf.getvalue()

ENG = engine.PowerPlanEngine()
N = int(sys.argv[1]) if len(sys.argv) > 1 else 12000
text = gen(N)
rows = pp_app.App.parseCsvText(text)
nodes = pp_app.App.nodesFromRows(rows)
print('节点 %d' % len(nodes), flush=True)
t = time.perf_counter()
lay = ENG.layout(nodes, False)
print('engine.layout %.1f ms' % ((time.perf_counter() - t) * 1000), flush=True)
t = time.perf_counter()
sc = ENG.scene(nodes, False, '大数据测试')
print('engine.scene %.1f ms' % ((time.perf_counter() - t) * 1000), flush=True)
t = time.perf_counter()
cc = ENG.countCrossings(lay['links'])
print('countCrossings %.1f ms -> %d' % ((time.perf_counter() - t) * 1000, cc), flush=True)
