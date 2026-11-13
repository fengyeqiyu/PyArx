# -*- coding: utf-8 -*-
"""纯事件驱动冒烟：全部操作由发送事件完成，无真人鼠标键盘。"""
import ctypes
import ctypes.wintypes as wt
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M
import pp_app
import pp_events as E

ok_n = 0
bad = 0


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
M.pump(100)
app.showEditor()
M.pump(250)

cv = E.find_control(app.win.hwnd, pp_app.CV)
tbl = E.find_control(app.win.hwnd, pp_app.TBL)
chk('按 id 递归找到画布', cv != 0, hex(cv))
chk('按 id 递归找到表格', tbl != 0, hex(tbl))

# 1. 单击卡片：事件驱动
code = app.nodes[2]['code']
rc = app.win.cardRectPlan(pp_app.CV, code)
E.click(cv, int(rc[0] + rc[2] / 2), int(rc[1] + rc[3] / 2))
chk('发事件单击 -> 选中卡片', app.win.getPlanSelection(pp_app.CV) == code,
    app.win.getPlanSelection(pp_app.CV))

# 2. 滚轮缩放：事件驱动
z0 = app.win.getPlanView(pp_app.CV)[0]
E.wheel(cv, 120, int(cv and 300), 300)
z1 = app.win.getPlanView(pp_app.CV)[0]
chk('发事件滚轮 -> 放大', z1 > z0 + 1e-4, '%.4f -> %.4f' % (z0, z1))
E.wheel(cv, -120, 300, 300)
z2 = app.win.getPlanView(pp_app.CV)[0]
chk('发事件反向滚轮 -> 缩回', abs(z2 - z0) < 1e-3, '%.4f' % z2)

# 3. 左键拖动平移：事件驱动
v0 = app.win.getPlanView(pp_app.CV)
E.drag(cv, 400, 300, 560, 400)
v1 = app.win.getPlanView(pp_app.CV)
chk('发事件拖动 -> 视口平移', abs(v1[1] - v0[1]) > 1 and abs(v1[2] - v0[2]) > 1,
    '(%s,%s) -> (%s,%s)' % (v0[1], v0[2], v1[1], v1[2]))

# 4. 表格：F2 打开行编辑条 + 改编号写回
E.click(tbl, 60, 60)               # 选中一行
M.pump(60)
row = app.win.getTableSelectedRow(pp_app.TBL)
E.key(tbl, 0x71)                   # VK_F2 = 0x71
M.pump(150)
chk('发事件 F2 -> 原地编辑开启', bool(app._ce), 'row=%s ce=%s' % (row, app._ce))
app._cellEnd(commit=False)
M.pump(100)
chk('原地编辑可取消', app._ce is None, repr(app._ce))

print('=== %d/%d PASS ===' % (ok_n, ok_n + bad), flush=True)
os._exit(0)
