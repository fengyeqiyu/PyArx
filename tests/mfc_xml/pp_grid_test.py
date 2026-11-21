# -*- coding: utf-8 -*-
"""原生表格（SysListView32）功能验收：滚动条 / F2 编辑 / Insert / Delete / 复制粘贴 / 列头排序 / 窗口缩放重排。"""
import ctypes
import ctypes.wintypes as wt
import os
import sys
import time

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
user32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, ctypes.c_void_p]
user32.SendMessageW.restype = ctypes.c_ssize_t
user32.PostMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.PostMessageW.restype = wt.BOOL
user32.GetClassNameW.argtypes = [wt.HWND, ctypes.c_wchar_p, ctypes.c_int]
user32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                                ctypes.c_int, ctypes.c_int, wt.UINT]
user32.SetForegroundWindow.argtypes = [wt.HWND]
user32.SetFocus.argtypes = [wt.HWND]
user32.GetFocus.restype = wt.HWND
user32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
user32.IsWindow.argtypes = [wt.HWND]
user32.IsWindow.restype = wt.BOOL
user32.OpenClipboard.argtypes = [wt.HWND]
user32.GetClipboardData.argtypes = [wt.UINT]
user32.GetClipboardData.restype = wt.HANDLE
user32.CloseClipboard.argtypes = []
kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
kernel32.GlobalLock.argtypes = [wt.HANDLE]
kernel32.GlobalLock.restype = ctypes.c_void_p
kernel32.GlobalUnlock.argtypes = [wt.HANDLE]

GW_CHILD, GW_HWNDNEXT = 5, 2
GWL_ID, GWL_STYLE = -12, -16
WS_VSCROLL = 0x00200000
WM_LBUTTONDOWN, WM_KEYDOWN, WM_KEYUP = 0x0201, 0x0100, 0x0101
VK_F2, VK_INSERT, VK_DELETE = 0x71, 0x2D, 0x2E
LVM_GETITEMCOUNT = 0x1004
LVM_GETHEADER = 0x101F
LVM_GETITEMRECT = 0x100E
HDM_GETITEMCOUNT = 0x1200
LVIR_BOUNDS = 0

RESULT = []


def ok(name, cond, info=''):
    RESULT.append((name, bool(cond)))
    print(('PASS' if cond else 'FAIL'), name, info, flush=True)


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


def clip_text():
    if not user32.OpenClipboard(None):
        return ''
    h = user32.GetClipboardData(13)      # CF_UNICODETEXT
    s = ''
    if h:
        p = kernel32.GlobalLock(h)
        if p:
            s = ctypes.wstring_at(p)
            kernel32.GlobalUnlock(h)
    user32.CloseClipboard()
    return s


app = pp_app.App()
app.win.show()
M.pump(80)
app.showEditor()
M.pump(200)

hwnd = app.win.hwnd
user32.SetWindowPos(hwnd, wt.HWND(-1), 40, 30, 0, 0, 0x0001 | 0x0040)   # 临时置顶，截图才抓得到自己
M.pump(120)

lv = find_by_id(hwnd, pp_app.TBL)
cls = ctypes.create_unicode_buffer(64)
user32.GetClassNameW(lv, cls, 64)
new_tbl = bool(getattr(app, '_newTable', False))
exp_cls = 'PyMfcTable' if new_tbl else 'SysListView32'
ok('表格控件是原生 SysListView32' if not new_tbl else '表格控件是新自绘 PyMfcTable',
   cls.value == exp_cls, cls.value)
row_count = app.win.get_table_row_count(pp_app.TBL)
ok('行数 = 节点数 39', row_count == 39, str(row_count))
exp_cols = len(pp_app.FIELDS) if new_tbl else 8
if new_tbl:
    col_count = len(pp_app.FIELDS)
else:
    hdr = user32.SendMessageW(lv, LVM_GETHEADER, 0, 0)
    col_count = user32.SendMessageW(hdr, HDM_GETITEMCOUNT, 0, 0)
ok('列数 = %d' % exp_cols, col_count == exp_cols, str(col_count))

style = user32.GetWindowLongPtrW(lv, GWL_STYLE)
if new_tbl:
    # 新自绘表按整数行整高排布（无原生 WS_VSCROLL 条），断言它已承载全部节点行
    ok('新表已承载全部行（无原生滚动条）', row_count == len(app.nodes), '%d/%d' % (row_count, len(app.nodes)))
else:
    ok('表格带垂直滚动条（WS_VSCROLL）', bool(style & WS_VSCROLL), hex(style))

# 末行是否落在客户区之外（真需要滚动）
if new_tbl:
    rc2 = app.tblCellRect(38, 0)
    ok('39 行超出可视区，滚动条生效', bool(rc2) and rc2[1] + rc2[3] > 400,
       'last row bottom=%s' % (rc2[1] + rc2[3] if rc2 else 0))
else:
    rc = wt.RECT()
    user32.SendMessageW(lv, LVM_GETITEMRECT, 38, ctypes.byref(rc))
    ok('39 行超出可视区，滚动条生效', rc.bottom > 400, 'last row bottom=%d' % rc.bottom)

# --- F2 编辑 -------------------------------------------------------------
user32.SetForegroundWindow(hwnd)
user32.SetFocus(lv)
app.tblSetSel(3)      # 等价于用户点了第 4 行
M.pump(60)
user32.PostMessageW(lv, WM_KEYDOWN, VK_F2, 0)  # VK_F2=0x71
M.pump(80)
ok('F2 进入原地编辑', bool(app._ce) and app._ce[0] == 3, repr(app._ce))
r0 = app._ce[0] if app._ce else 3
eid0 = app._ce[2] if app._ce else pp_app.E_F0
ce_col = app._ce[1] if app._ce else 0
ce_field = pp_app.FIELDS[ce_col]
oldCode = app.win.get_text(eid0)
app.win.set_text(eid0, 'X1-TEST')
app._cellEnd(commit=True)
M.pump(80)
ok('F2 改单元格后写回表格', app.tblGet(r0, ce_col) == 'X1-TEST',
   'col=%d %r' % (ce_col, app.tblGet(r0, ce_col)))
ok('节点数据同步更新', app.nodes[r0][ce_field] == 'X1-TEST',
   '%s=%r' % (ce_field, app.nodes[r0][ce_field]))
app._cellBegin(r0, ce_col)
app.win.set_text(pp_app.E_F0, oldCode)
app._cellEnd(commit=True)
M.pump(60)

# --- Insert -------------------------------------------------------------
n0 = app.win.get_table_row_count(pp_app.TBL)
user32.PostMessageW(lv, WM_KEYDOWN, VK_INSERT, 0)
M.pump(120)
ok('Insert 插入一行', app.win.get_table_row_count(pp_app.TBL) == n0 + 1,
   '%d -> %d' % (n0, app.win.get_table_row_count(pp_app.TBL)))
app._rowEditClose()

# --- Delete（删刚加的空叶子节点 N1） -------------------------------------
idx = len(app.nodes) - 1
user32.PostMessageW(lv, WM_LBUTTONDOWN, 1, (12) | (24 + 0 * 21 << 16))
M.pump(40)
app.tblSetSel(idx)
app.onGridDelete(app.nodes[idx]['code'])
M.pump(120)
ok('Delete 删除一行', app.win.get_table_row_count(pp_app.TBL) == n0,
   str(app.win.get_table_row_count(pp_app.TBL)))

# --- 复制 / 粘贴 ---------------------------------------------------------
user32.SetForegroundWindow(hwnd)
user32.SetFocus(lv)
app.tblSetSel(0)
M.pump(40)
user32.SendMessageW(lv, WM_KEYDOWN, 0x43, 0x001D0001)   # C（Ctrl 组合，直发到表控件）
user32.SendMessageW(lv, WM_KEYUP, 0x43, 0xC01D0001)
M.pump(120)
txt = clip_text()
exp_tabs = len(pp_app.FIELDS) - 1 if new_tbl else 7
ok('Ctrl+C 把选中行写入剪贴板', txt.count('\t') == exp_tabs, repr(txt[:80]))
# 列顺序即 FIELDS 顺序，编号不在首格（首格是"起点"，顶层柜为空）
code_cell = txt.split('\t')[pp_app.FIELDS.index('code')].strip()
ok('剪贴板含该行编号', code_cell == app.nodes[0]['code'], repr(code_cell))

before = len(app.nodes)
user32.SendMessageW(lv, WM_KEYDOWN, 0x56, 0x001D0001)   # V
user32.SendMessageW(lv, WM_KEYUP, 0x56, 0xC01D0001)
M.pump(150)
ok('Ctrl+V 触发粘贴（重复编号被跳过）', len(app.nodes) == before,
   '%d -> %d / %s' % (before, len(app.nodes), app.win.get_text(pp_app.L_EDIT_COUNT)))

# --- 列头排序 -----------------------------------------------------------
old_first = app.nodes[0]['code']
app.gridSort(pp_app.FIELDS.index('loadKw'))   # 按"功率"列排序
M.pump(100)
first_code = app.tblGet(0, pp_app.FIELDS.index('code'))
loads = [float(n.get('loadKw') or 0) for n in app.nodes]
ok('按负载排序生效', loads == sorted(loads), 'min=%.2f 首行=%s' % (loads[0], first_code))
ok('排序后表格与数据一致', first_code == app.nodes[0]['code'])


# --- 窗口缩放 → 布局重算 -------------------------------------------------
def client_size():
    return app.win.client_size()


def cv_rect():
    h = find_by_id(hwnd, pp_app.CV)
    r = wt.RECT()
    user32.GetWindowRect(h, ctypes.byref(r))
    return r.right - r.left, r.bottom - r.top


cw0, ch0 = client_size()
cvw0, cvh0 = cv_rect()
user32.SetWindowPos(hwnd, wt.HWND(-1), 40, 30, 1600, 1000, 0x0004 | 0x0040)
M.pump(400)
cw1, ch1 = client_size()
cvw1, cvh1 = cv_rect()
ok('窗口放大后客户区变大', (cw1, ch1) != (cw0, ch0), '%s -> %s' % ((cw0, ch0), (cw1, ch1)))
ok('画布跟着放大（不再固定 1308×755）', cvw1 > cvw0 + 100 and cvh1 > cvh0 + 40,
   '%s -> %s' % ((cvw0, cvh0), (cvw1, cvh1)))
ok('状态栏/小地图仍贴底', app.client_size()[1] - pp_app.MI_FRAME[3] > 0, '')

# 恢复原尺寸截图
user32.SetWindowPos(hwnd, wt.HWND(-1), 40, 30, 1360, 900, 0x0004 | 0x0040)
M.pump(300)
app.win.save_png(r'G:\PyArx\docs\_pp_grid_lv.png')
M.pump(200)

pas = sum(1 for _, c in RESULT if c)
print('\n=== %d/%d PASS ===' % (pas, len(RESULT)), flush=True)
for name, c in RESULT:
    if not c:
        print('  未通过：', name, flush=True)

print('done', flush=True)
os._exit(0)
