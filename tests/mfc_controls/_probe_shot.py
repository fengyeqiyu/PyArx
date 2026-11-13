import ctypes, struct, PyMfcXaml as p
from demo_level_tab import build
from _verify_level_tab import screen_shot, R
names={}
w=p.guiload(build(), names=names)
w.show()
u=ctypes.windll.user32
u.SetWindowPos.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_uint]
u.SetForegroundWindow.argtypes=[ctypes.c_void_p]
u.SetWindowPos(ctypes.c_void_p(w.hwnd), ctypes.c_void_p(-1), 60,60,0,0,0x0001|0x0040)
u.SetForegroundWindow(ctypes.c_void_p(w.hwnd))
p.pump(600)
rc=R(); u.GetWindowRect(ctypes.c_void_p(w.hwnd),ctypes.byref(rc))
print('rect',rc.l,rc.t,rc.r,rc.b)
screen_shot(rc.l, rc.t, rc.r-rc.l, rc.b-rc.t, r'G:\PyArx\docs\_lt_win.bmp')
