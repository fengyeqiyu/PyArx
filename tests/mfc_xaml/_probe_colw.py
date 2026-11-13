# -*- coding: utf-8 -*-
"""列宽热点探针：在真实 39x8 ListView 上单独计时 8 次 setColumnWidth。

口径：1324x841、软件 D2D、侧板 640（与基线一致）。
分三段：
  A) 裸调 8 次 setColumnWidth（宿主现状：每次带 InvalidateRect(FALSE)）
  B) 宽度未变 → 走跳过分支（对照）
  C) 整步 onSplitDrag（宽度真变）+ 紧随的 pump(30)
另逐列记录单次耗时，看是不是某一列特别贵。
"""
import os, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import PyMfcXaml as M
import pp_app

app = pp_app.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(400)
cw, ch = app._clientSize()
print('client=%dx%d 侧板宽=%d 行数=%d' % (cw, ch, app._sideWidth(cw), len(app.nodes)), flush=True)

TBL = pp_app.TBL

def widths_for(edw):
    inner = edw - 2 * pp_app.ED_PAD
    return app._gridColumns(inner)

# ---- A) 裸 8 次：每轮给一组不同宽度，逐列计时 ----
rounds = []
per_col = [[] for _ in range(8)]
base = app._sideWidth(cw)
for k in range(12):
    edw = base - k * 6
    ws = widths_for(edw)
    per_call = []
    t0 = time.perf_counter()
    for i, pxw in enumerate(ws):
        ta = time.perf_counter()
        app.win.setColumnWidth(TBL, i, pxw)
        per_call.append((time.perf_counter() - ta) * 1000.0)
    tot = (time.perf_counter() - t0) * 1000.0
    rounds.append(tot)
    for i, v in enumerate(per_call):
        per_col[i].append(v)
    M.pump(5)

def st(name, xs):
    xs = sorted(xs)
    print('%-24s n=%d  min %.2f  med %.2f  max %.2f  sum %.2f ms'
          % (name, len(xs), xs[0], xs[len(xs)//2], xs[-1], sum(xs)), flush=True)

st('A 裸8次合计/轮', rounds)
for i in range(8):
    st('  列%d 单次' % i, per_col[i])

# ---- B) 宽度未变：再调一次 _layout（应走跳过分支）----
b = []
for _ in range(12):
    t0 = time.perf_counter(); app._layout(); b.append((time.perf_counter() - t0) * 1000.0)
st('B _layout(宽度未变)', b)

# ---- C) 整步拖动：宽度真变，量 onSplitDrag 与 pump ----
dl, pm = [], []
for k in range(12):
    target = cw - (base - k * 6) - pp_app.SPLIT_W
    t0 = time.perf_counter(); app.onSplitDrag('move', target); t1 = time.perf_counter()
    dl.append((t1 - t0) * 1000.0)
    t2 = time.perf_counter(); M.pump(30); t3 = time.perf_counter()
    pm.append((t3 - t2) * 1000.0)
st('C onSplitDrag(宽度变)', dl)
st('C pump(30)', pm)

# 对照：宽度未变的 onSplitDrag（newW == _edW 时 _layout 被跳过）
dl2 = []
for _ in range(12):
    t0 = time.perf_counter(); app.onSplitDrag('move', cw - app._edW - pp_app.SPLIT_W)
    dl2.append((time.perf_counter() - t0) * 1000.0)
st("C' onSplitDrag(宽度不变)", dl2)

print('OK', flush=True)
os._exit(0)
