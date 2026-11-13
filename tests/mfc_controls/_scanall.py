import ctypes, sys
from ctypes import wintypes
u=ctypes.windll.user32
u.GetWindowTextW.argtypes=[ctypes.c_void_p,wintypes.LPWSTR,ctypes.c_int]
u.GetClassNameW.argtypes=[ctypes.c_void_p,wintypes.LPWSTR,ctypes.c_int]
u.GetWindowThreadProcessId.argtypes=[ctypes.c_void_p,ctypes.c_void_p]
EP=ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM)
def cb(h,_):
    c=ctypes.create_unicode_buffer(256); u.GetClassNameW(h,c,256)
    b=ctypes.create_unicode_buffer(512); u.GetWindowTextW(h,b,512)
    p=ctypes.c_ulong(0); u.GetWindowThreadProcessId(h,ctypes.byref(p))
    if c.value in ('PyMfcNativeWindow',) or 'PyArx' in b.value:
        print('pid=%d cls=%s title=%s' % (p.value, c.value, b.value))
    return True
u.EnumWindows(EP(cb),0)