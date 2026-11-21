# -*- coding: utf-8 -*-
"""诊断：直接用 loadDwgSvg 加载 tray_plan.svg，看 canvas 的 fit 缩放与尺寸"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M

svg = open(r'G:\PyArx\docs\tray_plan.svg', encoding='utf-8').read()
win = M.Window('桥架诊断', 1560, 740)
win.set_theme(False)
win.add_dwg_canvas(3001, 0, 0, 1560, 740)
win.load_dwg_svg(3001, svg)
win.show()
time.sleep(0.8)
print('zoom=%.4f' % win.get_dwg_zoom(3001))
win.save_png(r'G:\PyArx\docs\_tray_diag1.png')
win.set_dwg_view(3001, 1.0, -10, -10)
time.sleep(0.3)
win.save_png(r'G:\PyArx\docs\_tray_diag2.png')
win.close()
print('ok')
