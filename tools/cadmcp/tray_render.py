# -*- coding: utf-8 -*-
"""把 tray_canvas.xml 交给 PyMfcXml 的 XML 画布渲染成 PNG（验证"XML 画布能画出来"）"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M

XML = r'G:\PyArx\docs\tray_canvas.xml'
PNG = r'G:\PyArx\docs\tray_canvas_preview.png'

win = M.guiload(XML)
print('XML 画布已创建: %r' % (win.names() if hasattr(win, 'names') else '-'))
win.show()
time.sleep(0.8)
win.savePng(PNG)
time.sleep(0.4)
win.close()
print('png: %s (%d bytes)' % (PNG, os.path.getsize(PNG)))
