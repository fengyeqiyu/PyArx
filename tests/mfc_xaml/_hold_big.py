# -*- coding: utf-8 -*-
"""导入指定 CSV 后挂住窗口 45 s，供外部探针测表格滚动 / 画布平移缩放并截图。"""
import os, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pp_app, PyMfcXaml as M

path = sys.argv[1] if len(sys.argv) > 1 else r'G:\PyArx\docs\_big_20_10_50_allstart.csv'
text = open(path, encoding='utf-8').read()
app = pp_app.App()
app.win.show()
M.pump(80)
app.showEditor()
M.pump(150)
t0 = time.perf_counter()
app.importCsvText(text, os.path.basename(path))
app.loadMs = (time.perf_counter() - t0) * 1000
print('导入 %s %.1f ms' % (os.path.basename(path), app.loadMs), flush=True)
M.pump(300)
print('表格行数 %d' % len(app.tableRows()), flush=True)
end = time.time() + 45
while time.time() < end:
    M.pump(50)
