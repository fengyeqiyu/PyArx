# -*- coding: utf-8 -*-
"""用 XML 画布控件把 tray_plan.svg 渲染成预览图（loadDwgSvg + 自动 fit）"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXml as M

svg = open(r'G:\PyArx\docs\tray_plan.svg', encoding='utf-8').read()
win = M.Window('10KV 电缆桥架平面', 1560, 780)
win.set_theme(False)
win.add_dwg_canvas(7001, 8, 8, 1544, 764)
win.load_dwg_svg(7001, svg)
win.show()
time.sleep(0.9)
print('fit zoom = %.3f' % win.get_dwg_zoom(7001))
win.save_png(r'G:\PyArx\docs\tray_plan_preview.png')
win.close()
print('ok', os.path.getsize(r'G:\PyArx\docs\tray_plan_preview.png'))
