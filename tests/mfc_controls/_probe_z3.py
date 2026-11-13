import ctypes, PyMfcXaml as p
from demo_level_tab import build
names={}
w=p.guiload(build(), names=names); w.show(); p.pump(400)
u=ctypes.windll.user32
u.CreateWindowExW.argtypes=[ctypes.c_uint,ctypes.c_wchar_p,ctypes.c_wchar_p,ctypes.c_uint,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_void_p]
u.CreateWindowExW.restype=ctypes.c_void_p
u.GetWindow.argtypes=[ctypes.c_void_p,ctypes.c_uint]; u.GetWindow.restype=ctypes.c_void_p
u.GetDlgCtrlID.argtypes=[ctypes.c_void_p]; u.GetDlgCtrlID.restype=ctypes.c_int
u.SetWindowPos.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_uint]
host=u.GetWindow(ctypes.c_void_p(w.hwnd),5)
def chain():
    out=[]; c=u.GetWindow(ctypes.c_void_p(host),5)
    while c: out.append(u.GetDlgCtrlID(ctypes.c_void_p(c))); c=u.GetWindow(ctypes.c_void_p(c),2)
    return out
c1=u.CreateWindowExW(0,'Static','x1',0x40000000|0x10000000|0x800000,0,0,40,40,host,ctypes.c_void_p(9001),0,0)
c2=u.CreateWindowExW(0,'Static','x2',0x40000000|0x10000000|0x800000,20,20,40,40,host,ctypes.c_void_p(9002),0,0)
p.pump(200)
print('chain',chain())
print('ret',u.SetWindowPos(ctypes.c_void_p(c1),ctypes.c_void_p(1),0,0,0,0,0x1|0x2|0x3))
print('chain2',chain())
