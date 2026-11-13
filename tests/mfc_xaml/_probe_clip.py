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
WM_KEYDOWN, WM_KEYUP = 0x100, 0x101
VK_CONTROL = 0x11

app = pp_app.App(); app.win.show(); M.pump(80); app.showEditor(); M.pump(200)
lv = E.find_control(app.win.hwnd, pp_app.TBL)
u.SetFocus(lv)
app.win.setTableSelectedRow(pp_app.TBL, 0)
M.pump(40)

sel = u.SendMessageW(lv, 0x1000 + 1, -1, LVNI_SELECTED)  # LVM_GETNEXTITEM
print('selected item =', sel, flush=True)

# 方式 A：keybd_event（测试用法）
u.keybd_event(VK_CONTROL, 0, 0, 0)
u.keybd_event(0x43, 0, 0, 0)
u.keybd_event(0x43, 0, 2, 0)
u.keybd_event(VK_CONTROL, 0, 2, 0)
M.pump(120)

def clip():
    if not u.OpenClipboard(None): return 'OPEN_FAIL'
    h = u.GetClipboardData(13)
    s = ''
    if h:
        p = k.GlobalLock(h)
        if p:
            s = ctypes.wstring_at(p); k.GlobalUnlock(h)
    u.CloseClipboard(); return repr(s[:80])
print('after keybd_event ctrl state=', hex(u.GetKeyState(VK_CONTROL) & 0x8000), flush=True)
print('clip A =', clip(), flush=True)

# 方式 B：PostMessage WM_KEYDOWN（ctrl 键态由消息外无法设置，proc 里 GetKeyState 为 0）
u.PostMessageW(lv, WM_KEYDOWN, 0x43, 0)
M.pump(120)
print('clip B =', clip(), flush=True)
print('OK', flush=True)
os._exit(0)
