# -*- coding: utf-8 -*-
"""分割条验收：画布与侧板之间 6px 竖条，拖动改侧板宽度（360..900），画布跟着变。"""
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xml')

import PyMfcXml as M
import pp_app
import pp_events as E

import ctypes as C
import ctypes.wintypes as wt

u32 = C.windll.user32


def rect(hwnd):
    """控件矩形，相对主窗客户区（x, y, w, h）。"""
    rc = wt.RECT()
    pt = wt.POINT()
    u32.GetWindowRect(C.c_void_p(hwnd), C.byref(rc))
    pt.x, pt.y = rc.left, rc.top
    u32.ScreenToClient(C.c_void_p(app.win.hwnd), C.byref(pt))
    return (pt.x, pt.y, rc.right - rc.left, rc.bottom - rc.top)


ok_n, bad = 0, 0


def chk(name, cond, detail=''):
    global ok_n, bad
    if cond:
        ok_n += 1
        print('PASS %s %s' % (name, detail), flush=True)
    else:
        bad += 1
        print('FAIL %s %s' % (name, detail), flush=True)


app = pp_app.App()
app.win.show()
M.pump(200)
app.showEditor()
M.pump(400)

w = app.win
chk('侧板默认 640 宽', app._edW == pp_app.ED_W, 'edW=%d' % app._edW)

sp = E.find_control(w.hwnd, pp_app.SPLIT)
chk('分割条存在', sp is not None, repr(sp))

edX, edY, edw, edh = app._edRc
sx, sy, sw, sh = rect(sp)
chk('分割条紧贴侧板左边缘',
    sw == pp_app.SPLIT_W and sx + sw == edX and sh > 600,
    'split=(%d,%d,%dx%d) panel_x=%d' % (sx, sy, sw, sh, edX))

cv_w0 = app.cvW
# 向左拖 60px → 侧板变宽、画布变窄
E.drag(sp, 3, 200, 3 - 60, 200, steps=3)
M.pump(150)
chk('向左拖侧板变宽', app._edW >= 690, 'edW=%d' % app._edW)
chk('画布跟着变窄', app.cvW < cv_w0 - 40, 'cv %d -> %d' % (cv_w0, app.cvW))
sx2 = rect(sp)[0]
chk('分割条跟着左移', sx2 < sx, '%d -> %d' % (sx, sx2))

# 向右拖回 → 变窄（120px 位移只能收窄 120px，判据随起点走，不写死绝对值）
ed_w_before = app._edW
E.drag(sp, 3, 200, 3 + 120, 200, steps=3)
M.pump(150)
chk('向右拖侧板变窄', app._edW < ed_w_before, 'edW %d -> %d' % (ed_w_before, app._edW))

# 极值 clamp
E.drag(sp, 3, 200, 3 + 2000, 200, steps=3)
M.pump(150)
chk('最窄 clamp 360', app._edW == 360, 'edW=%d' % app._edW)
E.drag(sp, 3, 200, 3 - 2000, 200, steps=3)
M.pump(150)
chk('最宽 clamp 900', app._edW == 900, 'edW=%d cvW=%d' % (app._edW, app.cvW))

# 边界是算出来的，不是写死的：上限必须给画布留 CV_MIN_W
lo, hi = app._sideBounds(1324)
chk('窗口 1324 时侧板上限 = 900', hi == 900, 'lo=%d hi=%d' % (lo, hi))
lo2, hi2 = app._sideBounds(1000)
chk('窗口 1000 时上限收缩到给画布留 320',
    hi2 == 1000 - pp_app.CV_X - 8 - pp_app.SPLIT_W - 320, 'lo=%d hi=%d' % (lo2, hi2))
prev, app._edW = app._edW, 900
cw_small = 1000
chk('窄窗下生效宽度被钳住、偏好不动',
    app._sideWidth(cw_small) == hi2 and app._edW == 900,
    'eff=%d pref=%d' % (app._sideWidth(cw_small), app._edW))
app._edW = prev

# 画布与侧板不重叠（分割条夹在中间）
sx3, sy3, sw3, sh3 = rect(sp)
p1 = rect(E.find_control(w.hwnd, pp_app.CV))
chk('画布右边界 = 分割条左边界', p1[0] + p1[2] <= sx3, 'cv=%dx%d split_x=%d' % (p1[0], p1[2], sx3))

print('=== %d/%d PASS ===' % (ok_n, ok_n + bad), flush=True)
try:
    w.savePng(r'G:\PyArx\docs\_split.png')
    print('截图 _split.png', flush=True)
except Exception as e:
    print('截图失败 %s' % e, flush=True)
os._exit(0)
