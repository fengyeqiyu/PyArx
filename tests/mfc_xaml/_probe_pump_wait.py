# -*- coding: utf-8 -*-
"""等待语义回归：空队列下 pump(ms) 应按预算等待，而不是 0~2ms 立即返回。"""
import os, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import PyMfcXaml as M
import pp_app

app = pp_app.App(); app.win.show(); M.pump(100); app.showEditor(); M.pump(100)

# 用例里代表性预算：cursor 20/120、click 80、cell 20；另测 200 长等待。
for ms in (20, 80, 120, 200):
    t0 = time.perf_counter()
    M.pump(ms)
    el = (time.perf_counter() - t0) * 1000
    print('pump(%3d) 实测 %6.1f ms' % (ms, el), flush=True)

os._exit(0)
