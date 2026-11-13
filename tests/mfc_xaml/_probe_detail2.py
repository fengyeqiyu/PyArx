# -*- coding: utf-8 -*-
"""对照：重复弹出详情时，跳过 raiseMany（z 序已固定）的收益；并量 raiseMany 内部分段。"""
import os, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import PyMfcXaml as M
import pp_app

app = pp_app.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(400)
w = app.win
code = app.nodes[1]['code']

# 预热
app.fillDetail(code); M.pump(60)

def patched_show(self):
    self._detailVisible = True
    self._layout()

def run(label, skip_raise, n=8):
    fill_t, pump_t = [], []
    orig_show = app.showDetail
    for _ in range(n):
        app.hideDetail(); M.pump(20); app.curSel = None
        if skip_raise:
            app.showDetail = patched_show.__get__(app)
        t0 = time.perf_counter(); app.fillDetail(code); t1 = time.perf_counter()
        fill_t.append((t1 - t0) * 1000.0)
        t2 = time.perf_counter(); M.pump(30); t3 = time.perf_counter()
        pump_t.append((t3 - t2) * 1000.0)
        app.showDetail = orig_show
    def st(name, xs):
        xs = sorted(xs)
        print('%-26s n=%d  min %.2f  med %.2f  max %.2f ms  total %.1f'
              % (name, len(xs), xs[0], xs[len(xs)//2], xs[-1], sum(xs)), flush=True)
    print('== %s ==' % label, flush=True)
    st('fillDetail(同步)', fill_t)
    st('pump(30)', pump_t)
    st('fill+pump', [a+b for a, b in zip(fill_t, pump_t)])

run('现状（每次 raiseMany）', False)
run('重复选中跳过 raiseMany', True)

print('OK', flush=True)
os._exit(0)
