import ctypes
from ctypes import wintypes
u=ctypes.windll.user32
u.EnumWindows.argtypes=[ctypes.c_void_p,wintypes.LPARAM]
u.GetWindowTextW.argtypes=[ctypes.c_void_p,wintypes.LPWSTR,ctypes.c_int]
u.GetWindowThreadProcessId.argtypes=[ctypes.c_void_p,ctypes.c_void_p]
EP=ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM)
titles=[]
def cb(h,_):
    b=ctypes.create_unicode_buffer(512); u.GetWindowTextW(h,b,512)
    pid=ctypes.c_ulong(0); u.GetWindowThreadProcessId(h,ctypes.byref(pid))
    if b.value and pid.value==60344: titles.append((h,b.value))
    return True
u.EnumWindows(EP(cb),0)
for h,t in titles: print(hex(h),t)