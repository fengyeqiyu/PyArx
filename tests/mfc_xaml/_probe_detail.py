# -*- coding: utf-8 -*-
"""详情弹出耗时拆分：fillDetail / showDetail / _layout / raiseMany / 原生控件写值 / pump。"""
import os, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import PyMfcXaml as M
import pp_app

app = pp_app.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(400)
cw, ch = app._clientSize()
print('client=%dx%d 侧板宽=%d' % (cw, ch, app._sideWidth(cw)), flush=True)

code = app.nodes[1]['code']

# 预热：先弹一次
app.fillDetail(code); M.pump(60)

# 拆 fillDetail 内部：先把 showDetail 摘出来量
w = app.win
times = dict(fill=[], show=[], layout=[], raise_many=[], setvals=[], pump=[])

for r in range(8):
    app.hideDetail(); M.pump(20)
    app.curSel = None

    t0 = time.perf_counter()
    # ---- 复刻 fillDetail 的原生写值部分（不含 showDetail）----
    n = app.nodeOf(code)
    t1 = time.perf_counter()
    w.setText(pp_app.DT_TITLE, '节点 %s' % n['code'])
    w.setText(pp_app.D_F0 + 0, n['code'])
    w.setText(pp_app.D_F0 + 1, n.get('name') or '')
    w.setText(pp_app.D_F0 + 2, n.get('upstream') or '')
    w.setText(pp_app.D_F0 + 3, n.get('cable') or '')
    cur = pp_app.normVolt(n.get('voltage'))
    opts = [('380', '380V 三相'), ('220', '220V 单相'), ('10kV', '10kV 中压'),
            ('35kV', '35kV 中压'), ('110kV', '110kV 高压')]
    if cur and all(o[0] != cur for o in opts):
        opts.append((cur, pp_app.voltText(cur)))
    w.clearStrings(pp_app.D_VOL)
    for o in opts:
        w.addString(pp_app.D_VOL, o[1])
    w.setCurSel(pp_app.D_VOL, next((i for i, o in enumerate(opts) if o[0] == cur), 0))
    w.setText(pp_app.D_LEN, '%g' % float(n.get('length') or 0))
    w.setText(pp_app.D_LOAD, '%g' % float(n.get('loadKw') or 0))
    w.setText(pp_app.DT_TIP, 'tip')
    t2 = time.perf_counter()
    times['setvals'].append((t2 - t1) * 1000.0)

    t3 = time.perf_counter()
    app.showDetail()
    t4 = time.perf_counter()
    times['show'].append((t4 - t3) * 1000.0)
    times['fill'].append((time.perf_counter() - t0) * 1000.0)

    t5 = time.perf_counter(); M.pump(30); t6 = time.perf_counter()
    times['pump'].append((t6 - t5) * 1000.0)

# 单独拆 showDetail 内部：_layout 与 raiseMany
for r in range(8):
    app.hideDetail(); M.pump(20)
    app._detailVisible = True
    t0 = time.perf_counter(); app._layout(); t1 = time.perf_counter()
    times['layout'].append((t1 - t0) * 1000.0)
    t2 = time.perf_counter(); w.raiseMany(app._detailIds); t3 = time.perf_counter()
    times['raise_many'].append((t3 - t2) * 1000.0)

def st(name, xs):
    xs = sorted(xs)
    print('%-16s n=%d  min %.2f  med %.2f  max %.2f ms'
          % (name, len(xs), xs[0], xs[len(xs)//2], xs[-1]), flush=True)

for k in ('setvals', 'show', 'fill', 'layout', 'raise_many', 'pump'):
    st(k, times[k])

# 详情控件数量 & 显隐状态
print('detail ids = %d' % len(app._detailIds), flush=True)
print('OK', flush=True)
os._exit(0)
