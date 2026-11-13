# -*- coding: utf-8 -*-
"""表格性能基线：把布局/填表/选中/滚动的耗时分别量出来，只报数字。"""
import os, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M
import pp_app

app = pp_app.App()
app.win.show(); M.pump(200)
t0 = time.perf_counter(); app.showEditor(); M.pump(400); t1 = time.perf_counter()
print('showEditor+pump  %.1f ms' % ((t1 - t0) * 1000))

def t(label, fn, n=5):
    fn()
    ts = []
    for _ in range(n):
        a = time.perf_counter(); fn(); b = time.perf_counter(); ts.append((b - a) * 1000)
    print('%-28s min %.1f  med %.1f  max %.1f ms' % (label, min(ts), sorted(ts)[len(ts)//2], max(ts)))

t('_layout()', lambda: app._layout(), 10)
t('fillTable()', lambda: app.fillTable(), 5)
t('collectGrid()', lambda: app.collectGrid(), 5)
t('updateStatus()', lambda: app.updateStatus(), 10)

print('nodes=%d rows=%d cols=%d' % (len(app.nodes), len(app.nodes), 8))

# 选中一行 + 光标移动到下一行（模拟键盘/点击滚动）
tbl = None
for _ in range(2):
    pass
def sel():
    app.win.setTableSelectedRow(pp_app.TBL, 5)
t('setTableSelectedRow(5)', sel, 10)

a = time.perf_counter()
for r in range(len(app.nodes)):
    app.win.setTableSelectedRow(pp_app.TBL, r)
b = time.perf_counter()
print('逐行选中 39 行总计 %.1f ms' % ((b - a) * 1000))
os._exit(0)
