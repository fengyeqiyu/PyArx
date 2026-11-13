# -*- coding: utf-8 -*-
"""量三条：分割条拖动的单帧重排成本、节点点击到详情弹出的耗时、分割线是否可见。"""
import os, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M
import pp_app

app = pp_app.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(400)
cw, ch = app.clientSize()
print('client=%dx%d 侧板宽=%d' % (cw, ch, app._sideWidth(cw)), flush=True)

# A) 拖动：连发 40 个 move，看真正重排了几帧、每帧多久
t_all = time.perf_counter()
applied = [0]
orig = app.layout
def timed(drag=False):
    t = time.perf_counter(); n = orig(drag); d = time.perf_counter() - t
    applied.append(d); return n
app.layout = timed
w0 = app._sideWidth(cw)
for i in range(40):
    app.onSplitDrag('move', cw - w0 - pp_app.SPLIT_W - 100 + i * 10)
    M.pump(3)
app.onSplitDrag('up', cw - w0 - pp_app.SPLIT_W - 100 + 390)
total = (time.perf_counter() - t_all) * 1000
durs = [d for d in applied if d]
print('拖动40步：总 %0.1f ms，重排 %d 帧，单帧 %0.2f ms（最大 %0.2f）'
      % (total, len(durs), sum(durs)/len(durs) if durs else 0, max(durs) if durs else 0), flush=True)
app.layout = orig

# B) 点节点 → 详情弹出
code = app.nodes[1]['code']
tt = []
for _ in range(5):
    app.curSel = None
    t = time.perf_counter(); app.fillDetail(code); M.pump(30); tt.append((time.perf_counter() - t) * 1000)
print('点节点→详情 %.1f ms（5 次均值）visible=%s cur=%s'
      % (sum(tt)/len(tt), app._detailVisible, app.curSel), flush=True)

# C) 分割线控件是否存在且可见
has = hasattr(app.win, 'setVisible')
rc = app.win.getPos(pp_app.SPLIT) if hasattr(app.win, 'getPos') else None
print('分割线 id=%d 热区 id=%d' % (pp_app.L_SPLIT if hasattr(pp_app,'L_SPLIT') else -1, pp_app.SPLIT), flush=True)
print('OK', flush=True)
os._exit(0)
