# -*- coding: utf-8 -*-
"""灌入 12000 行后挂住窗口 70 秒，供外部探针测表格滚动性能。"""
import os, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pp_app, PyMfcXaml as M

text = open(r'G:\PyArx\docs\_big.csv', 'r', encoding='utf-8-sig').read()
app = pp_app.App()
app.win.show()
M.pump(80)
app.showEditor()
M.pump(150)
t0 = time.perf_counter()
app.importCsvText(text, '12000 行测试')
print('导入 12000 行 %.1f ms' % ((time.perf_counter() - t0) * 1000), flush=True)
M.pump(300)
print('表格行数 %d' % len(app.tableRows()), flush=True)
end = time.time() + 70
while time.time() < end:
    M.pump(50)
