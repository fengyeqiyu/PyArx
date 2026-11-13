import ctypes, PyMfcXaml as p
from demo_level_tab import build
from _verify_level_tab import screen_shot, R, find_by_id
names={}
w=p.guiload(build(), names=names)
w.show()
u=ctypes.windll.user32
u.SetWindowPos.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_uint]
u.SetForegroundWindow.argtypes=[ctypes.c_void_p]
u.SetWindowPos(ctypes.c_void_p(w.hwnd), ctypes.c_void_p(-1), 60,60,0,0,0x0001|0x0040)
u.SetForegroundWindow(ctypes.c_void_p(w.hwnd)); p.pump(600)
rc=R(); u.GetWindowRect(ctypes.c_void_p(w.hwnd),ctypes.byref(rc))
b=find_by_id(w,names['b']); a=find_by_id(w,names['a'])
print('b',hex(b),'a',hex(a))
u.SetWindowPos(ctypes.c_void_p(b),0,0,0,0,0,0x0001|0x0002|0x0004|0x0010)  # b to top
p.pump(300)
screen_shot(rc.l,rc.t,rc.r-rc.l,rc.b-rc.t,r'G:\PyArx\docs\_lt_s3.bmp')
