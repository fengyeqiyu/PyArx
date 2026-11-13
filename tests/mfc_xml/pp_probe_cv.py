# -*- coding: utf-8 -*-
"""定位主画布 1000×744 下纸面/卡片落点异常：只放一个画布，给出坐标实测。"""
import os, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXml as M
import pp_app

nodes = pp_app.sampleThreeLevel()
plan = M.powerplan.build(nodes, M.powerplan.Style(), 'probe', False)
print('plan %dx%d' % (plan['width'], plan['height']))

win = M.Window('probe', 1200, 800)
win.setTheme(False)
win.addPowerCanvas(4001, 8, 72, 1000, 744)
win.loadPowerPlan(4001, plan['json'])
print('loadPowerPlan 后 view =', win.getPlanView(4001))
win.setPlanView(4001, 0.2, -117, 329)
print('setPlanView 后 view =', win.getPlanView(4001))
for code in ('TR1', 'AT1', 'AT1-1', 'AT1-1-1'):
    print('  cardRect', code, win.cardRectPlan(4001, code))
win.show()
time.sleep(0.4)
M.pump(30)
win.savePng(r'G:\PyArx\docs\_pp_probe_cv.png')
print('截图 _pp_probe_cv.png', os.path.getsize(r'G:\PyArx\docs\_pp_probe_cv.png'))
win.close()
