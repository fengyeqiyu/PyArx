import sys, ctypes
from ctypes import wintypes
u=ctypes.windll.user32
u.GetWindowTextW.argtypes=[ctypes.c_void_p,wintypes.LPWSTR,ctypes.c_int]
u.GetWindowThreadProcessId.argtypes=[ctypes.c_void_p,ctypes.c_void_p]
u.GetClassNameW.argtypes=[ctypes.c_void_p,wintypes.LPWSTR,ctypes.c_int]
u.IsWindowVisible.argtypes=[ctypes.c_void_p]
u.GetParent.argtypes=[ctypes.c_void_p]; u.GetParent.restype=ctypes.c_void_p
EP=ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM)
def walk(h,depth):
    b=ctypes.create_unicode_buffer(512); u.GetWindowTextW(h,b,512)
    c=ctypes.create_unicode_buffer(256); u.GetClassNameW(h,c,256)
    pid=ctypes.c_ulong(0); u.GetWindowThreadProcessId(h,ctypes.byref(pid))
    if b.value and ('PyArx' in b.value or 'PY-MFC' in b.value or 'PALETTE' in b.value.upper()):
        print('%s%s [%s] vis=%d parent=%s' % ('  '*depth, b.value, c.value, u.IsWindowVisible(h), hex(u.GetParent(h) or 0)))
    def cb(ch,_):
        walk(ch,depth+1); return True
    u.EnumChildWindows(h,EP(cb),0)
def top(h,_):
    pid=ctypes.c_ulong(0); u.GetWindowThreadProcessId(h,ctypes.byref(pid))
    if pid.value==int(__import__("sys").argv[1]): walk(h,0)
    return True
u.EnumWindows(EP(top),0)