# -*- coding: utf-8 -*-
"""MfcXml 画布加载局部细节 XScene 并存高清 PNG。"""
import os, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXml as M

XML = r'G:\PyArx\examples\cad2d\dwg1018\dwg_detail.xml'
PNG = r'G:\PyArx\examples\cad2d\dwg1018\mfc_detail.png'

with open(XML, 'r', encoding='utf-8') as f:
    xml = f.read()
print('XML 长度 %d' % len(xml))

win = M.Window('DWG 局部细节 · MfcXml 画布', 1280, 720)
win.set_theme(False)
win.add_dwg_canvas(3002, 0, 0, 1280, 720)
win.load_dwg_xml(3002, xml)
win.show()
time.sleep(0.6)
print('fit zoom = %.4f' % win.get_dwg_zoom(3002))
win.save_png(PNG)
print('png', PNG)
time.sleep(0.3)
win.close()
