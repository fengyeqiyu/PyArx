# -*- coding: utf-8 -*-
"""画布交互与控件导出验收：点卡片 / 点连线 / 控件级 PNG。"""
import ctypes
import ctypes.wintypes as wt
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXaml as M
import pp_app

user32 = ctypes.WinDLL('user32', use_last_error=True)
user32.GetWindow.argtypes = [wt.HWND, wt.UINT]
user32.GetWindow.restype = wt.HWND
user32.GetWindowLongPtrW.argtypes = [wt.HWND, ctypes.c_int]
user32.GetWindowLongPtrW.restype = ctypes.c_ssize_t
user32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.SendMessageW.restype = ctypes.c_ssize_t
user32.PostMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.PostMessageW.restype = wt.BOOL
user32.SetForegroundWindow.argtypes = [wt.HWND]

GW_CHILD, GW_HWNDNEXT, GWL_ID = 5, 2, -12
WM_MOUSEMOVE, WM_LBUTTONDOWN, WM_LBUTTONUP = 0x0200, 0x0201, 0x0202

ok_all = 0
bad = 0


def chk(name, cond, detail=''):
    global ok_all, bad
    if cond:
        ok_all += 1
        print('PASS %s %s' % (name, detail), flush=True)
    else:
        bad += 1
        print('FAIL %s %s' % (name, detail), flush=True)


def find_by_id(parent, cid):
    h = user32.GetWindow(parent, GW_CHILD)
    while h:
        if user32.GetWindowLongPtrW(h, GWL_ID) == cid:
            return h
        sub = find_by_id(h, cid)
        if sub:
            return sub
        h = user32.GetWindow(h, GW_HWNDNEXT)
    return None


def click(hwnd, x, y):
    user32.SendMessageW(hwnd, WM_LBUTTONDOWN, 1, x | (y << 16))
    M.pump(20)
    user32.SendMessageW(hwnd, WM_LBUTTONUP, 0, x | (y << 16))
    M.pump(80)


app = pp_app.App()
app.win.show()
M.pump(100)
app.showEditor()
M.pump(250)

cv = find_by_id(app.win.hwnd, pp_app.CV)
print('画布 %s' % cv, flush=True)

# --- 1. 点卡片：拿卡片屏幕矩形 -> 中心点击 ---
code0 = app.nodes[2]['code']
rc = app.win.cardRectPlan(pp_app.CV, code0)
print('卡片 %s 客户区 rect=%s' % (code0, rc), flush=True)
if rc and len(rc) == 4 and rc[2] > 0:
    cx, cy = int(rc[0] + rc[2] / 2), int(rc[1] + rc[3] / 2)
    click(cv, cx, cy)
    sel = app.win.getPlanSelection(pp_app.CV)
    chk('单击卡片被选中', sel == code0, '%s (点击 %d,%d)' % (sel, cx, cy))
    chk('详情浮层打开', bool(app._detailVisible), 'visible=%s cur=%s'
        % (app._detailVisible, app.curSel))
    chk('详情显示的就是该节点', app.curSel == code0, app.curSel)
else:
    chk('单击卡片被选中', False, 'planCardRect 取不到')

# --- 2. 点空白处：应清空选中 ---
click(cv, 3, 3)
chk('点空白清空选中', app.win.getPlanSelection(pp_app.CV) == '',
    repr(app.win.getPlanSelection(pp_app.CV)))

# --- 3. 点连线：取某条线的中点（模型坐标按当前 view 折算成客户区） ---
# 详情浮层若还在屏上会吃掉这一下（planHit 先判 detailHit），先显式收起再取视口参数。
app.hideDetail()
M.pump(150)
view = app.win.getPlanView(pp_app.CV)
zoom, ox, oy = view[0], view[1], view[2]
hit = None
for l in app.view.get('links', [])[:6]:
    print('link', l, flush=True)
    mid_x = (l['bezier'][0] + l['bezier'][6]) / 2.0
    mid_y = (l['bezier'][1] + l['bezier'][7]) / 2.0
    # 取折线（不是贝塞尔）中点：C++ 绘制与命中都按 bezier 的 4 个拐点连成正交折线，
    # 用三次贝塞尔 t=0.5 的点算出来会偏离 8px 命中容差，导致偶发选不中。
    pts = [(l['bezier'][q * 2], l['bezier'][q * 2 + 1]) for q in range(4)]
    segs = []
    total = 0.0
    for a, b in zip(pts, pts[1:]):
        ln = ((b[0] - a[0]) ** 2 + (b[1] - a[1]) ** 2) ** 0.5
        segs.append((a, b, ln))
        total += ln
    want, acc = total / 2.0, 0.0
    px, py = pts[0]
    for a, b, ln in segs:
        if ln > 0 and acc + ln >= want:
            t = (want - acc) / ln
            px = a[0] + (b[0] - a[0]) * t
            py = a[1] + (b[1] - a[1]) * t
            break
        acc += ln
    hit = (int(ox + px * zoom), int(oy + py * zoom), l['to'])
    break
if hit:
    click(cv, hit[0], hit[1])
    sel2 = app.win.getPlanSelection(pp_app.CV)
    chk('单击连线选中下游节点', sel2 == hit[2], '%s (期望 %s, 点击 %d,%d)'
        % (sel2, hit[2], hit[0], hit[1]))
else:
    chk('单击连线选中下游节点', False, 'view 里没有 links')

# --- 4. 控件级导出 PNG ---
p1 = r'G:\PyArx\docs\_ctl_canvas.png'
p2 = r'G:\PyArx\docs\_ctl_table.png'
r1 = app.win.saveControlPng(pp_app.CV, p1)
r2 = app.win.saveControlPng(pp_app.TBL, p2)
chk('画布控件导出 PNG', bool(r1) and os.path.exists(p1),
    '%s B' % (os.path.getsize(p1) if os.path.exists(p1) else 0))
chk('表格控件导出 PNG', bool(r2) and os.path.exists(p2),
    '%s B' % (os.path.getsize(p2) if os.path.exists(p2) else 0))

print('=== %d/%d PASS ===' % (ok_all, ok_all + bad), flush=True)
os._exit(0)
