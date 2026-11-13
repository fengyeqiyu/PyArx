import sys, ctypes
from ctypes import wintypes
u=ctypes.windll.user32
u.GetWindowTextW.argtypes=[ctypes.c_void_p,wintypes.LPWSTR,ctypes.c_int]
u.GetClassNameW.argtypes=[ctypes.c_void_p,wintypes.LPWSTR,ctypes.c_int]
u.GetWindowThreadProcessId.argtypes=[ctypes.c_void_p,ctypes.c_void_p]
u.IsWindowVisible.argtypes=[ctypes.c_void_p]
u.GetWindowRect.argtypes=[ctypes.c_void_p,ctypes.c_void_p]
class R(ctypes.Structure):
    _fields_=[('l',ctypes.c_long),('t',ctypes.c_long),('r',ctypes.c_long),('b',ctypes.c_long)]
EP=ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM)
pid=int(sys.argv[1]); rows=[]
def cb(h,_):
    c=ctypes.create_unicode_buffer(256); u.GetClassNameW(h,c,256)
    b=ctypes.create_unicode_buffer(512); u.GetWindowTextW(h,b,512)
    p=ctypes.c_ulong(0); u.GetWindowThreadProcessId(h,ctypes.byref(p))
    if p.value==pid and c.value=='PyMfcNativeWindow':
        r=R(); u.GetWindowRect(h,ctypes.byref(r))
        rows.append((hex(h),b.value,u.IsWindowVisible(h),r.l,r.t,r.r-r.l,r.b-r.t))
    return True
u.EnumWindows(EP(cb),0)
for x in rows: print(x)
print('total',len(rows))