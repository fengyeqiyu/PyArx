# -*- coding: utf-8 -*-
import ctypes, os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release'); sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXaml as M, ppApp

u = ctypes.WinDLL('user32', use_last_error=True)
u.SetFocus.argtypes=[ctypes.c_void_p]; u.SetFocus.restype=ctypes.c_void_p
u.GetFocus.restype=ctypes.c_void_p
u.GetClassNameW.argtypes=[ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_int]
u.GetWindow.argtypes=[ctypes.c_void_p, ctypes.c_uint]; u.GetWindow.restype=ctypes.c_void_p
u.GetWindowLongPtrW.argtypes=[ctypes.c_void_p, ctypes.c_int]; u.GetWindowLongPtrW.restype=ctypes.c_ssize_t

app = ppApp.App(); app.win.show(); M.pump(80); app.showEditor(); M.pump(200)

def find(parent, cid):
    h = u.GetWindow(parent, 5)
    while h:
        if u.GetWindowLongPtrW(h, -12) == cid: return h
        s = find(h, cid)
        if s: return s
        h = u.GetWindow(h, 2)
    return None

lv = find(app.win.hwnd, ppApp.TBL)
print('lv=', hex(lv or 0), 'new=', app._newTable)
u.SetFocus(lv); M.pump(20)
print('focus=', hex(u.GetFocus()), 'same=', u.GetFocus()==lv)
buf = ctypes.create_unicode_buffer(64); u.GetClassNameW(u.GetFocus(), buf, 64)
print('focus cls=', buf.value)
os._exit(0)
