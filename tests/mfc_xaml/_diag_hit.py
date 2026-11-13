# -*- coding: utf-8 -*-
"""命中测试直连诊断：hitPlan / cardRectPlan / 选区。"""
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXaml as M
import pp_app

app = pp_app.App()
app.win.show()
M.pump(100)
app.showEditor()
M.pump(250)

code = app.nodes[2]['code']
rc = app.win.cardRectPlan(pp_app.CV, code)
view = app.win.getPlanView(pp_app.CV)
print('code=%s rect=%s view=%s' % (code, rc, view), flush=True)

node = [n for n in app.view['nodes'] if n['code'] == code][0]
print('模型 box: x=%s y=%s w=%s h=%s' % (node.get('x'), node.get('y'),
                                         node.get('w'), node.get('h')), flush=True)

for (x, y) in ((140, 389), (130, 390), (150, 395), (135, 387)):
    print('hitPlan(%d,%d) -> %r' % (x, y, app.win.hitPlan(pp_app.CV, x, y)), flush=True)

print('hitPlan 卡片中心 ->', repr(app.win.hitPlan(
    pp_app.CV, int(rc[0] + rc[2] / 2), int(rc[1] + rc[3] / 2))), flush=True)
print('selectPlan ->', app.win.selectPlan(pp_app.CV, code), flush=True)
print('selection ->', repr(app.win.getPlanSelection(pp_app.CV)), flush=True)
os._exit(0)
