# -*- coding: utf-8 -*-
"""拆分 fillDetail 非泵内成本与 pump(30) 成本。"""
import os, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import PyMfcXaml as M
import pp_app

app = pp_app.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(300)
code = app.nodes[1]['code']
app.fillDetail(code); M.pump(300)

A, B = [], []
for _ in range(8):
    app.curSel = None
    t0 = time.perf_counter(); app.fillDetail(code); t1 = time.perf_counter()
    A.append((t1 - t0) * 1000)
    M.pump(30)
    t2 = time.perf_counter(); B.append((t2 - t1) * 1000)

def med(x): return sorted(x)[len(x)//2]
print('fillDetail 非泵 med %.2f mean %.2f' % (med(A), sum(A)/len(A)))
print('pump(30)    med %.2f mean %.2f' % (med(B), sum(B)/len(B)))
print('合计 med %.2f' % (med(A) + med(B)))
t0=time.perf_counter(); M.pump(30); print('空 pump %.2f' % ((time.perf_counter()-t0)*1000))
os._exit(0)
