# -*- coding: utf-8 -*-
"""诊断：直接用 loadPowerSvg 加载 tray_plan.svg，看 canvas 的 fit 缩放与尺寸"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M

svg = open(r'G:\PyArx\docs\tray_plan.svg', encoding='utf-8').read()
win = M.Window('桥架诊断', 1560, 740)
win.setTheme(False)
win.addPowerCanvas(3001, 0, 0, 1560, 740)
win.loadPowerSvg(3001, svg)
win.show()
time.sleep(0.8)
print('zoom=%.4f' % win.getPlanZoom(3001))
win.savePng(r'G:\PyArx\docs\_tray_diag1.png')
win.setPlanView(3001, 1.0, -10, -10)
time.sleep(0.3)
win.savePng(r'G:\PyArx\docs\_tray_diag2.png')
win.close()
print('ok')
