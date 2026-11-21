# -*- coding: utf-8 -*-
"""把 strong3d_tray.svg 用 XML 画布控件（loadDwgSvg）渲染成预览 PNG"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXml as M

svg = open(r'G:\PyArx\docs\strong3d_tray.svg', encoding='utf-8').read()
win = M.Window('强电桥架三维示意', 1620, 900)
win.set_theme(False)
win.add_dwg_canvas(7001, 8, 8, 1604, 884)
win.load_dwg_svg(7001, svg)
win.show()
time.sleep(0.9)
print('fit zoom = %.3f' % win.get_dwg_zoom(7001))
win.save_png(r'G:\PyArx\docs\strong3d_tray_preview.png')
win.close()
print('png %d bytes' % os.path.getsize(r'G:\PyArx\docs\strong3d_tray_preview.png'))
