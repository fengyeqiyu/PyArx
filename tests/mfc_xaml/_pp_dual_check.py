# -*- coding: utf-8 -*-
"""临时验证：节点写双上游（起点 AT2-1|AT1-3）后，运行态是否会掉出树、卡片正文是否为新口径。"""
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXaml as M
import pp_app

app = pp_app.App()
app.win.show()
M.pump(80)
for n in app.nodes:
    if n['code'] == 'AT2-1-3':
        n['upstream'] = 'AT2-1|AT1-3'
app.rebuild(fit=True)
M.pump(250)
boxes = app.view.get('boxes') or []
row = [b for b in boxes if b['code'] == 'AT2-1-3']
links = app.view.get('links') or []
print('节点数=%d 卡数=%d' % (len(app.nodes), len(boxes)), flush=True)
print('AT2-1-3 坐标=%s' % row, flush=True)
print('连线=%d 其中 dashed=%d' % (len(links), sum(1 for l in links if l.get('dashed')),
                                  ), flush=True)
app.showEditor()
M.pump(200)
app.win.savePng(r'G:\PyArx\docs\_pp_dual_check.png')
M.pump(80)
print('已截图 docs/_pp_dual_check.png', flush=True)
