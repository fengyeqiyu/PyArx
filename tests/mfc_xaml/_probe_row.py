# -*- coding: utf-8 -*-
"""差 1 探针：同一次真实点击同时量 4 个数。
① 点击客户区 y；② LVM_SUBITEMHITTEST 的 iItem/iSubItem；
③ LVM_GETNEXTITEM 真实选中行；④ cellRect(TBL,row,col) 矩形。
另打表头高、行矩形 tops、表格父原点，定位偏差出在哪一步。
口径：1324x841、软件 D2D、侧板 640。"""
import os, sys, ctypes, ctypes.wintypes as wt
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import PyMfcXaml as M
import pp_app, pp_events as E

u = ctypes.windll.user32
k = ctypes.windll.kernel32

LVM_FIRST = 0x1000
LVM_GETHEADER = LVM_FIRST + 31
LVM_GETITEMRECT = LVM_FIRST + 4
LVM_SUBITEMHITTEST = LVM_FIRST + 6
LVM_GETNEXTITEM = LVM_FIRST + 1
LVNI_SELECTED = 0x0002

class LVHITTEST(ctypes.Structure):
    _fields = [('pt', wt.POINT), ('flags', wt.UINT), ('iItem', ctypes.c_int),
                ('iSubItem', ctypes.c_int), ('iX', ctypes.c_int), ('iY', ctypes.c_int)]

class LVITEMW(ctypes.Structure):
    _fields = [('mask', wt.UINT), ('iItem', ctypes.c_int), ('iSubItem', ctypes.c_int),
                ('stateMask', wt.UINT), ('state', wt.UINT), ('pszText', wt.LPWSTR),
                ('cchTextMax', ctypes.c_int), ('iImage', ctypes.c_int),
                ('lParam', ctypes.c_void_p)]

def item_rect(h, row):
    rc = wt.RECT(); rc.left = rc.top = rc.right = rc.bottom = 0
    u.SendMessageW(h, LVM_GETITEMRECT, row, ctypes.byref(rc))
    return rc

app = pp_app.App(); app.win.show(); M.pump(80)
app.showEditor(); M.pump(200)
lv = E.find_control(app.win.hwnd, pp_app.TBL)
u.SetForegroundWindow(app.win.hwnd); u.SetFocus(lv)

rowH, hdr = app.win.getTableMetrics(pp_app.TBL)
print(f'metrics rowH={rowH} hdr={hdr}', flush=True)

for r in (0, 1, 2):
    rr = app.win.cellRect(pp_app.TBL, r, 0)
    print(f'cellRect row {r}: {rr}', flush=True)

# ③ 点击前选中
sel0 = u.SendMessageW(lv, LVM_GETNEXTITEM, -1, LVNI_SELECTED)
print(f'③ before-click native sel={sel0} host={app.win.getTableSelectedRow(pp_app.TBL)}', flush=True)

u.SetForegroundWindow(app.win.hwnd)
for tgt in (1, 3, 5):
    tx, ty, tw, th = app.win.cellRect(pp_app.TBL, tgt, 0)
    cx = tx + tw // 2
    cy = ty + th // 2
    sp = wt.POINT(cx, cy); u.ClientToScreen(lv, ctypes.byref(sp))
    u.SetCursorPos(sp.x, sp.y); M.pump(20)
    u.mouse_event(0x0002, 0, 0, 0, 0)
    u.mouse_event(0x0004, 0, 0, 0, 0)
    M.pump(120)
    hi = LVHITTEST(); hi.pt = wt.POINT(cx, cy)
    u.SendMessageW(lv, LVM_SUBITEMHITTEST, 0, ctypes.byref(hi))
    sel = u.SendMessageW(lv, LVM_GETNEXTITEM, -1, LVNI_SELECTED)
    py_sel = app.win.getTableSelectedRow(pp_app.TBL)
    rr = app.win.cellRect(pp_app.TBL, sel, 0)
    print(f'>> real-click row {tgt} pt=({cx},{cy}) screen=({sp.x},{sp.y}) | hit={hi.iItem} | native={sel} host={py_sel} | cellRect(native)={rr}', flush=True)

print('app._tblPos =', app._tblPos, flush=True)

# 表格真实行首格文本（核对行号对应数据）
for r in (0, 1, 2):
    buf = ctypes.create_unicode_buffer(256)
    itm = LVITEMW(); itm.iSubItem = 0; itm.pszText = ctypes.cast(buf, wt.LPWSTR); itm.cchTextMax = 256
    u.SendMessageW(lv, LVM_FIRST + 73, r, ctypes.byref(itm))  # GETITEMTEXTW
    print(f'data row {r}: {buf.value}', flush=True)
print('OK', flush=True)
os._exit(0)

