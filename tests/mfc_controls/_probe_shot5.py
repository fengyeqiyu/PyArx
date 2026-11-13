import ctypes, PyMfcXaml as p
from demo_level_tab import build
from _verify_level_tab import screen_shot, R
names={}
w=p.guiload(build(), names=names); w.show()
u=ctypes.windll.user32
u.SetWindowPos.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_uint]
u.SetForegroundWindow.argtypes=[ctypes.c_void_p]
u.GetWindow.argtypes=[ctypes.c_void_p,ctypes.c_uint]; u.GetWindow.restype=ctypes.c_void_p
u.GetDlgCtrlID.argtypes=[ctypes.c_void_p]; u.GetDlgCtrlID.restype=ctypes.c_int
u.SetWindowPos(ctypes.c_void_p(w.hwnd),ctypes.c_void_p(-1),60,60,0,0,0x1|0x40); u.SetForegroundWindow(ctypes.c_void_p(w.hwnd)); p.pump(500)
rc=R(); u.GetWindowRect(ctypes.c_void_p(w.hwnd),ctypes.byref(rc))
def chain():
    host=u.GetWindow(ctypes.c_void_p(w.hwnd),5); out=[]; c=u.GetWindow(ctypes.c_void_p(host),5)
    while c: out.append(u.GetDlgCtrlID(ctypes.c_void_p(c))); c=u.GetWindow(ctypes.c_void_p(c),2)
    return out
w.setLevel(names['a'], 0); w.setLevel(names['b'], 5); p.pump(300)
print('a,b =', names['a'], names['b'], 'chain', chain())
screen_shot(rc.l,rc.t,rc.r-rc.l,rc.b-rc.t, r'G:\PyArx\docs\_lt_t1.bmp')
w.setLevel(names['b'], -5); p.pump(300)
print('after bottom chain', chain())
screen_shot(rc.l,rc.t,rc.r-rc.l,rc.b-rc.t, r'G:\PyArx\docs\_lt_t2.bmp')
