# -*- coding: utf-8 -*-
"""详情帧阶段拆分：控件赋值 / showDetail(layout+raise) / pump(30)。"""
import os, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import PyMfcXaml as M
import pp_app

app = pp_app.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(300)
n = app.nodes[1]
app.fillDetail(n['code']); M.pump(300)  # 首帧：建 z 序、灌下拉

win = app.win
A, B, C, T, L = [], [], [], [], []
for _ in range(5):
    app.cur_sel = None
    t0 = time.perf_counter()
    win.setText(pp_app.DT_TITLE, '节点 %s' % n['code'])
    win.setText(pp_app.D_F0 + 0, n['code'])
    win.setText(pp_app.D_F0 + 1, n.get('name') or '')
    win.setText(pp_app.D_F0 + 2, n.get('upstream') or '')
    win.setText(pp_app.D_F0 + 3, n.get('cable') or '')
    win.setCurSel(pp_app.D_VOL, 0)
    win.setText(pp_app.D_LEN, '12.5')
    win.setText(pp_app.D_LOAD, '0')
    win.setText(pp_app.DT_TIP, '第 2 层 · 下游 18 个')
    t1 = time.perf_counter(); A.append((t1 - t0) * 1000)

    tl = time.perf_counter(); app.layout(); tl2 = time.perf_counter()
    L.append((tl2 - tl) * 1000)
    app.showDetail()
    t2 = time.perf_counter(); B.append((t2 - t1) * 1000)

    M.pump(30)
    t3 = time.perf_counter(); C.append((t3 - t2) * 1000)
    T.append((t3 - t0) * 1000)

def med(x): return sorted(x)[len(x)//2]
print('控件赋值 med %.2f  mean %.2f' % (med(A), sum(A)/len(A)))
print('showDetail med %.2f  mean %.2f' % (med(B), sum(B)/len(B)))
print('  其中 layout med %.2f mean %.2f' % (med(L), sum(L)/len(L)))
print('pump(30)    med %.2f  mean %.2f' % (med(C), sum(C)/len(C)))
print('整帧        med %.2f  mean %.2f' % (med(T), sum(T)/len(T)))

# 空泵对照
t0 = time.perf_counter(); M.pump(30); print('空 pump(30) %.2f' % ((time.perf_counter()-t0)*1000))
os._exit(0)
