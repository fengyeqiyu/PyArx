# -*- coding: utf-8 -*-
"""大数据量导入 + 表格滚动性能实测：12000 行计算书 CSV。"""
import csv, io, os, sys, time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pp_app
import PyMfcXaml as M

def gen(count):
    buf = io.StringIO()
    wr = csv.writer(buf, lineterminator='\r\n')
    wr.writerow(pp_app.COLS[:pp_app.BOOK_N])
    for i in range(count):
        wr.writerow([
            'TR1', 'WL-%05d' % i, 'AT-%05d' % i, 'YJV22-4×95', '380V 三相',
            1, '%.1f' % (10 + i % 20), '2', '2', '%d' % (13 + i % 20), '3.0',
        ])
    return buf.getvalue()

app = pp_app.App()
app.win.show()
M.pump(80)
app.showEditor()
M.pump(150)

N = int(sys.argv[1]) if len(sys.argv) > 1 else 12000
text = gen(N)

t0 = time.perf_counter()
rows = pp_app.App.parseCsvText(text)
t1 = time.perf_counter()
nodes = app.nodesFromRows(rows)
t2 = time.perf_counter()
print('CSV %d 行：解析 %.1f ms / 转节点 %.1f ms' % (N, (t1 - t0) * 1000, (t2 - t1) * 1000), flush=True)

t3 = time.perf_counter()
app.loadNodes(nodes, '大数据测试')
t4 = time.perf_counter()
print('loadNodes（含布局+灌表） %.1f ms' % ((t4 - t3) * 1000), flush=True)
M.pump(200)

t5 = time.perf_counter()
app.fillTable()
t6 = time.perf_counter()
print('setTableRows 重灌 %d 行 %.1f ms' % (len(app.tableRows()), (t6 - t5) * 1000), flush=True)
M.pump(200)

print('表格行数 %d 画布 %sx%s' % (len(app.tableRows()), app.cvW, app.cvH), flush=True)
app.win.savePng(r'G:\PyArx\docs\_big_view.png')
M.pump(150)
print('done', flush=True)
M.pump(3000)
