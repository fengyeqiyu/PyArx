# -*- coding: utf-8 -*-
import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
sys.path.insert(0, r'G:\PyArx\pylibs')
import PyMfcXaml as M
import pp_app

app = pp_app.App(); app.win.show(); M.pump(120); app.showEditor(); M.pump(300)
zoom, ox, oy = app.win.getPlanView(pp_app.CV)
status = app.win.getText(pp_app.L_STATUS)
print('view', app.view)
print('viewArea', app.viewArea())
print('zoom', zoom, 'ox', ox, 'oy', oy)
print('status', status)
if app.view:
    W, H = app.view['width'], app.view['height']
    aw, ah = app.viewArea()
    raw = min((aw - 8) / W, (ah - 8) / H)
    print('expected raw', raw, 'clamped', max(0.05, min(4.0, raw)))
os._exit(0)
