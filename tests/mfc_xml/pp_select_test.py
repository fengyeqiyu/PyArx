# -*- coding: utf-8 -*-
"""画布交互与控件导出验收：点卡片 / 点连线 / 控件级 PNG。"""
import ctypes
import ctypes.wintypes as wt
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M
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
user32.GetClientRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
user32.GetClientRect.restype = wt.BOOL

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


def wait_canvas_ready(timeout_ms=2500, step_ms=100):
    """等画布视口稳定：zoom/offset 连续两次采样一致才算布局/动画跑完。

    进入编辑器的头几百毫秒里视口还会因布局/自适应缩放变化，此时算出来的卡片矩形
    和点击时的变换不是同一个，点击会落在卡片外（批跑时表现为偶发"单击卡片未选中"）。
    """
    last = None
    waited = 0
    while waited < timeout_ms:
        cur = app.win.get_dwg_view(pp_app.CV)
        if last is not None and cur == last:
            return cur
        last = cur
        M.pump(step_ms)
        waited += step_ms
    return last


wait_canvas_ready()

cv = find_by_id(app.win.hwnd, pp_app.CV)
print('画布 %s' % cv, flush=True)


def client_size(hwnd):
    rc = wt.RECT()
    user32.GetClientRect(hwnd, ctypes.byref(rc))
    return rc.right - rc.left, rc.bottom - rc.top


def click_card(code, tries=4):
    """点卡片：每轮重新取矩形再点。

    视口在进入编辑器后还会被自适应缩放/居中改一次（ox/oy 能差上百像素），
    先前算好的矩形在 20ms 后点下去就可能落在卡片外，所以按需重取重试。
    """
    for _ in range(tries):
        rc = app.win.card_rect_dwg(pp_app.CV, code)
        if not rc or len(rc) != 4 or rc[2] <= 0:
            M.pump(100)
            continue
        cx, cy = int(rc[0] + rc[2] / 2), int(rc[1] + rc[3] / 2)
        click(cv, cx, cy)
        if app.win.get_dwg_selection(pp_app.CV) == code:
            return cx, cy
        M.pump(100)
    return None


# --- 1. 点卡片：拿卡片屏幕矩形 -> 中心点击 ---
code0 = app.nodes[2]['code']
print('卡片 %s 客户区 rect=%s' % (code0, app.win.card_rect_dwg(pp_app.CV, code0)), flush=True)
hit = click_card(code0)
if hit:
    cx, cy = hit
    chk('单击卡片被选中', True, '%s (点击 %d,%d)' % (code0, cx, cy))
    chk('详情浮层打开', bool(app._detailVisible), 'visible=%s cur=%s'
        % (app._detailVisible, app.curSel))
    chk('详情显示的就是该节点', app.curSel == code0, app.curSel)
else:
    chk('单击卡片被选中', False, '四次重点都没选中 %s' % code0)

# --- 2. 点空白处：应清空选中 ---
# 空白点不能写死坐标：画布里有连线，坐标偶尔落在线段上（点线=选中其下游节点），
# 边角 (3,3) 也不在命中判定的有效区域。这里按网格试几个点，命中空白即通过。
w, h = client_size(cv)
cleared_at = None
for fx, fy in ((0.06, 0.06), (0.94, 0.06), (0.06, 0.94), (0.94, 0.94),
               (0.5, 0.06), (0.5, 0.94), (0.06, 0.5), (0.94, 0.5)):
    px, py = int(w * fx), int(h * fy)
    app.hideDetail()
    M.pump(80)
    click(cv, px, py)
    if app.win.get_dwg_selection(pp_app.CV) == '':
        cleared_at = (px, py)
        break
chk('点空白清空选中', cleared_at is not None,
    '在 %s 处清空' % (cleared_at,) if cleared_at else '八个候选空白点都没清空')

# --- 3. 点连线：取某条线的中点（模型坐标按当前 view 折算成客户区） ---
# 详情浮层若还在屏上会吃掉这一下（hitDwg 先判 detailHit），先显式收起再取视口参数。
app.hideDetail()
M.pump(150)
view = app.win.get_dwg_view(pp_app.CV)
for l in app.view.get('links', [])[:6]:
    print('link', l, flush=True)
    break


def click_link(tries=5):
    """点连线选下游节点：每轮重新取视口参数再算点击点。

    批跑时画布首次绘制晚于取视口参数，zoom/ox/oy 可能还是默认值，
    按旧值折算出来的点会离线 8px 命中容差之外，表现为偶发选不中。
    """
    last = None
    for _ in range(tries):
        view = app.win.get_dwg_view(pp_app.CV)
        zoom, ox, oy = view[0], view[1], view[2]
        if zoom <= 0:
            M.pump(120)
            continue
        links = app.view.get('links', [])[:6]
        if not links:
            return None
        l = links[0]
        # 取折线（不是贝塞尔）中点：C++ 绘制与命中都按 bezier 的 4 个拐点连成正交折线，
        # 用三次贝塞尔 t=0.5 的点算出来会偏离命中容差。
        pts = [(l['bezier'][q * 2], l['bezier'][q * 2 + 1]) for q in range(4)]
        segs, total = [], 0.0
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
        click(cv, hit[0], hit[1])
        M.pump(60)
        sel2 = app.win.get_dwg_selection(pp_app.CV)
        last = (hit, sel2)
        if sel2 == hit[2]:
            return last
        M.pump(140)
    return last


link_hit = click_link()
if link_hit:
    hit, sel2 = link_hit
    chk('单击连线选中下游节点', sel2 == hit[2], '%s (期望 %s, 点击 %d,%d)'
        % (sel2, hit[2], hit[0], hit[1]))
else:
    chk('单击连线选中下游节点', False, 'view 里没有 links 或视口尚未就绪')

# --- 4. 控件级导出 PNG ---
p1 = r'G:\PyArx\docs\_ctl_canvas.png'
p2 = r'G:\PyArx\docs\_ctl_table.png'
r1 = app.win.save_control_png(pp_app.CV, p1)
r2 = app.win.save_control_png(pp_app.TBL, p2)
chk('画布控件导出 PNG', bool(r1) and os.path.exists(p1),
    '%s B' % (os.path.getsize(p1) if os.path.exists(p1) else 0))
chk('表格控件导出 PNG', bool(r2) and os.path.exists(p2),
    '%s B' % (os.path.getsize(p2) if os.path.exists(p2) else 0))

print('=== %d/%d PASS ===' % (ok_all, ok_all + bad), flush=True)
os._exit(0)
