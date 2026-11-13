# -*- coding: utf-8 -*-
import os, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import PyMfcXaml as M
import pp_app

app = pp_app.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(400)
w = app.win
ids = app._detailIds

# 第一次建立 z 序
w.raiseMany(ids); M.pump(60)

# 连续多次 raiseMany：若同序短路，第二次起应接近 0
for r in range(8):
    t = time.perf_counter(); w.raiseMany(ids); dt = (time.perf_counter()-t)*1000
    M.pump(30)
    print('raiseMany #%d 同步 %.2f ms' % (r+1, dt), flush=True)

print('OK', flush=True)
os._exit(0)
