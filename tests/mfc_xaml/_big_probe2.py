# -*- coding: utf-8 -*-
"""大数据量导入分段计时：定位 12000 行里哪一段最贵。"""
import csv, io, os, sys, time, json

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pp_app
import engine
ENG = engine.PowerPlanEngine()
import PyMfcXaml as M

def gen(count):
    buf = io.StringIO()
    wr = csv.writer(buf, lineterminator='\r\n')
    wr.writerow(pp_app.COLS[:pp_app.BOOK_N])
    for i in range(count):
        wr.writerow(['TR1', 'WL-%05d' % i, 'AT-%05d' % i, 'YJV22-4×95', '380V 三相',
                     1, '%.1f' % (10 + i % 20), '2', '2', '%d' % (13 + i % 20), '3.0'])
    return buf.getvalue()

app = pp_app.App()
app.win.show()
M.pump(80)
app.showEditor()
M.pump(150)

N = int(sys.argv[1]) if len(sys.argv) > 1 else 12000
t = lambda: time.perf_counter()
a = t(); text = gen(N); b = t(); print('生成 CSV %.1f ms' % ((b - a) * 1000), flush=True)
a = t(); rows = pp_app.App.parseCsvText(text); b = t(); print('parseCsvText %.1f ms (%d 行)' % ((b - a) * 1000, len(rows)), flush=True)
a = t(); nodes = app.nodesFromRows(rows); b = t(); print('nodesFromRows %.1f ms (%d 节点)' % ((b - a) * 1000, len(nodes)), flush=True)

app.nodes = nodes
a = t(); vn = app.layoutNodes(); b = t(); print('layoutNodes %.1f ms (%d)' % ((b - a) * 1000, len(vn)), flush=True)
a = t(); scene = ENG.scene(vn, app.horiz, '大数据测试'); b = t(); print('ENG.scene %.1f ms' % ((b - a) * 1000), flush=True)
a = t(); cross = ENG.countCrossings(scene['layout']['links']); b = t(); print('countCrossings %.1f ms' % ((b - a) * 1000), flush=True)
a = t(); stext = json.dumps(scene, ensure_ascii=False); b = t(); print('json.dumps %.1f ms (%d KB)' % ((b - a) * 1000, len(stext) // 1024), flush=True)
a = t(); app.win.loadPowerScene(pp_app.CV, stext); b = t(); print('loadPowerScene(CV) %.1f ms' % ((b - a) * 1000), flush=True)
a = t(); app.win.loadPowerScene(pp_app.MINI, stext); b = t(); print('loadPowerScene(MINI) %.1f ms' % ((b - a) * 1000), flush=True)
a = t(); app.fillTable(); b = t(); print('fillTable %.1f ms' % ((b - a) * 1000), flush=True)
M.pump(300)
print('done', flush=True)
M.pump(2500)
