# -*- coding: utf-8 -*-
"""MfcXml 画布加载离线解码 XScene：起窗口、loadDwgXml、存 PNG 核验。"""
import os, sys, time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M

XML = r'G:\PyArx\examples\cad2d\dwg1018\dwg_scene.xml'
PNG = r'G:\PyArx\examples\cad2d\dwg1018\mfc_view.png'

with open(XML, 'r', encoding='utf-8') as f:
    xml = f.read()
print('XML 长度 %d' % len(xml))

win = M.Window('DWG 离线解码 · MfcXml 画布', 1280, 860)
win.set_theme(False)
win.add_dwg_canvas(3001, 0, 0, 1280, 860)
win.load_dwg_xml(3001, xml)
win.show()
time.sleep(0.6)

print('fit zoom = %.4f' % win.get_dwg_zoom(3001))
win.save_png(PNG)
print('png', PNG)
time.sleep(0.3)
win.close()
