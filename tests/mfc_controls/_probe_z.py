import ctypes, PyMfcXaml as p
from demo_level_tab import build
from _verify_level_tab import find_by_id
names={}
w=p.guiload(build(), names=names); w.show(); p.pump(400)
u=ctypes.windll.user32
u.GetWindow.argtypes=[ctypes.c_void_p,ctypes.c_uint]; u.GetWindow.restype=ctypes.c_void_p
u.GetDlgCtrlID.argtypes=[ctypes.c_void_p]; u.GetDlgCtrlID.restype=ctypes.c_int
u.SetWindowPos.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_uint]; u.SetWindowPos.restype=ctypes.c_int
u.GetParent.argtypes=[ctypes.c_void_p]; u.GetParent.restype=ctypes.c_void_p
def chain(h):
    out=[]; c=u.GetWindow(ctypes.c_void_p(h),5)
    while c:
        out.append((u.GetDlgCtrlID(ctypes.c_void_p(c)), c))
        c=u.GetWindow(ctypes.c_void_p(c),2)
    return out
host=u.GetWindow(ctypes.c_void_p(w.hwnd),5)
a=find_by_id(w,names['a'])
print('before', [(i,hex(h)) for i,h in chain(host)])
print('ret', u.SetWindowPos(ctypes.c_void_p(a), ctypes.c_void_p(1), 0,0,0,0, 0x1|0x2|0x4))
print('after ', [(i,hex(h)) for i,h in chain(host)])
print('ret2', u.SetWindowPos(ctypes.c_void_p(a), ctypes.c_void_p(0), 0,0,0,0, 0x1|0x2|0x4))
print('after2', [(i,hex(h)) for i,h in chain(host)])
