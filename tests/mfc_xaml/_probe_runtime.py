# -*- coding: utf-8 -*-
"""A/B 运行时取证：新表类名、能力探测、pyd 时间戳；命中/选中/单元格矩形五数对齐。"""
import os, sys, time, ctypes as C, ctypes.wintypes as wt
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import PyMfcXaml as M
import pp_app

# pyd 路径 + 时间戳
import PyMfcXaml, os, datetime
pyd = PyMfcXaml.__file__
mt = datetime.datetime.fromtimestamp(os.path.getmtime(pyd))
print('PYD=%s mtime=%s' % (pyd, mt), flush=True)

app = pp_app.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(300)

# A1: id=3021 子窗口类名
u32 = C.windll.user32
hw = app.win._hwnd if hasattr(app.win, '_hwnd') else None
# 找 TBL 控件
tbl = pp_app.TBL
def find_child(parent, target):
    res = []
    WNDENUMPROC = C.WINFUNCTYPE(C.c_int, wt.HWND, wt.LPARAM)
    def cb(h, lp):
        if C.windll.user32.GetDlgCtrlID(h) == target:
            res.append(h)
        return 1
    u32.EnumChildWindows(parent, WNDENUMPROC(cb), 0)
    return res[0] if res else None

thw = find_child(app.win.hwnd, tbl)
cls = C.createUnicodeBuffer(64)
u32.GetClassNameW(thw, cls, 64)
print('TBL hwnd=%d class=%s' % (thw or 0, cls.value), flush=True)

# A2: 能力探测
print('_newTable=%s  hasattr(addTable)=%s  hasattr(tableCellRect)=%s'
      % (bool(app._newTable), hasattr(app.win, 'addTable'), hasattr(app.win, 'tableCellRect')), flush=True)

# B: 行号/选中/单元格矩形五数（第 3 行，第 1 列）
row, col = 2, 1
if app._newTable:
    sel = app.win.tableSelectedRow(tbl)
    rc = app.win.tableCellRect(tbl, row, col)
    print('B: row=%d selectedRow=%s cellRect(x,y,w,h)=%s' % (row, sel, rc), flush=True)
    # 控件父原点 + 单元格矩形 = 编辑框绝对位置（不再加表头 24px）
    pr = C.wintypes.RECT(); u32.GetWindowRect(thw, C.byref(pr))
    ax, ay = pr.left + rc[0], pr.top + rc[1]
    print('B: 控件父原点=(%d,%d) 编辑框绝对=(%d,%d) 行矩形 top=%d' % (pr.left, pr.top, ax, ay, rc[1]), flush=True)

assert cls.value == 'PyMfcTable', '未接线：仍是 %s' % cls.value
assert app._newTable and hasattr(app.win, 'addTable') and hasattr(app.win, 'tableCellRect')
print('ASSERT OK: 新表已接线，无静默回落', flush=True)
os._exit(0)
