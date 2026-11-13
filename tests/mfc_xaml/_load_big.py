# -*- coding: utf-8 -*-
"""把大表 CSV 真实走一遍「导入」链路（importCsvText → loadNodes），计时 + 出画布截图。"""
import os, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pp_app
import PyMfcXaml as M

path = sys.argv[1] if len(sys.argv) > 1 else r'G:\PyArx\docs\_big_20_10_50.csv'
tag = os.path.basename(path).replace('.csv', '').replace('_', '')

app = pp_app.App()
app.win.show()
M.pump(80)
app.showEditor()
M.pump(150)

t0 = time.perf_counter()
text = open(path, encoding='utf-8').read()
t1 = time.perf_counter()
app.importCsvText(text, os.path.basename(path))
t2 = time.perf_counter()
M.pump(200)

print('read %.0f ms | importCsvText %.0f ms | 画布 %sx%s | 状态栏 %s' %
      ((t1 - t0) * 1000, (t2 - t1) * 1000, app.cvW, app.cvH,
       app.win.getText(pp_app.L_STATUS)), flush=True)
app.win.savePng(r'G:\PyArx\docs\_imp_%s.png' % tag)
M.pump(300)
print('done', flush=True)
M.pump(3500)
