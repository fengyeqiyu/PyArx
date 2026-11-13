import ctypes, PyMfcXaml as p
from demo_level_tab import build
names={}
w=p.guiload(build(), names=names)
w.show(); p.pump(300)
print("names:", names)
u=ctypes.windll.user32
u.GetWindow.argtypes=[ctypes.c_void_p,ctypes.c_uint]; u.GetWindow.restype=ctypes.c_void_p
u.GetDlgCtrlID.argtypes=[ctypes.c_void_p]; u.GetDlgCtrlID.restype=ctypes.c_int
u.GetClassNameW.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int]
def kids(h):
    out=[]; c=u.GetWindow(ctypes.c_void_p(h),5)
    while c:
        out.append((u.GetDlgCtrlID(ctypes.c_void_p(c)), c))
        c=u.GetWindow(ctypes.c_void_p(c),2)
    return out
print("top hwnd", w.hwnd)
print("top kids", [(i,hex(h)) for i,h in kids(w.hwnd)])
inner=h=u.GetWindow(ctypes.c_void_p(w.hwnd),5)
print("inner id", u.GetDlgCtrlID(ctypes.c_void_p(h)))
print("inner kids", [(i,hex(hh)) for i,hh in kids(h)])
