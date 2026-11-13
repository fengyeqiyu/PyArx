# -*- coding: utf-8 -*-
import os, sys, ctypes, ctypes.wintypes as wt
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import PyMfcXaml as M
import pp_app, pp_events as E

u = ctypes.windll.user32
k = ctypes.windll.kernel32
LVNI_SELECTED = 0x0002
LVM_GETNEXTITEM = 0x1001

app = pp_app.App(); app.win.show(); M.pump(80); app.showEditor(); M.pump(200)
lv = E.find_control(app.win.hwnd, pp_app.TBL)
u.SetForegroundWindow(app.win.hwnd)
u.SetFocus(lv)
app.win.setTableSelectedRow(pp_app.TBL, 0)
M.pump(40)

print('foreground=', hex(u.GetForegroundWindow()), 'focus=', hex(u.GetFocus()), 'lv=', hex(lv), flush=True)
sel = []
it = -1
while True:
    it = u.SendMessageW(lv, LVM_GETNEXTITEM, it, LVNI_SELECTED)
    if it < 0: break
    sel.append(it)
print('selected =', sel, flush=True)

# 枚举选中行首格文本
for r in sel:
    buf = ctypes.create_unicode_buffer(512)
    itm = wt.LVITEMW(); itm.iSubItem = 0; itm.pszText = buf; itm.cchTextMax = 512
    u.SendMessageW(lv, 0x1000 + 11, r, ctypes.byref(itm))  # LVM_GETITEMTEXTW
    print('row', r, 'text', buf.value, flush=True)

print('OK', flush=True)
os._exit(0)
