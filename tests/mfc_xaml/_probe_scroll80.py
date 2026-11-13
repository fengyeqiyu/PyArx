# -*- coding: utf-8 -*-
"""80 行滚动探针：追加 80 行，ensureVisible(79) 后第 80 行必须落在可视区。"""
import sys, os
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M
import pp_app
import pp_events as E

TBL = pp_app.TBL

app = pp_app.App()
app.win.show()
M.pump(200)
app.showEditor()
M.pump(400)
w = app.win

for i in range(80):
    w.appendTableRow(TBL, ['N%d' % i, 'name%d' % i, 'up', 'cable', '12.5', '380V', str(i * 2)])
M.pump(120)

rows = w.tableRowCount(TBL)
view = w.tableViewRows(TBL)
print('rowCount=%d viewRows=%d' % (rows, view))

# 滚到最后一行（数据行 rows-1）
last = rows - 1
w.tableEnsureVisible(TBL, last)
M.pump(120)
r79 = w.tableCellRect(TBL, last, 0)
print('rectLast(row=%d)=%s' % (last, r79))

# 滚回顶部，第 1 行 top 应等于表头高 24
w.tableEnsureVisible(TBL, 0)
M.pump(120)
r0 = w.tableCellRect(TBL, 0, 0)
print('rect0=%s' % (r0,))

# 滚轮向下 6 格：首行被滚出可视区，rect0 应为空
tbl = E.find_control(w.hwnd, TBL)
for _ in range(6):
    E.wheel(tbl, -120, 200, 200, pump=20)
M.pump(120)
r0b = w.tableCellRect(TBL, 0, 0)
print('after-wheel rect0=%s' % (r0b,))

ok = rows >= 80 and r79 and r79[1] >= 24 and r79[1] < 700 and r0[1] == 24 and r0b[1] < 24
print('PASS' if ok else 'FAIL')
sys.exit(0 if ok else 1)
