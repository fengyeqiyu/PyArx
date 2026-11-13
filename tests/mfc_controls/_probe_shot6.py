import ctypes, PyMfcXaml as p
from demo_level_tab import build
from _verify_level_tab import screen_shot, R, zorder
names={}
w=p.guiload(build(), names=names); w.show()
u=ctypes.windll.user32
u.SetWindowPos.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_uint]
u.SetForegroundWindow.argtypes=[ctypes.c_void_p]
u.SetWindowPos(ctypes.c_void_p(w.hwnd),ctypes.c_void_p(-1),60,60,0,0,0x1|0x40); u.SetForegroundWindow(ctypes.c_void_p(w.hwnd)); p.pump(600)
rc=R(); u.GetWindowRect(ctypes.c_void_p(w.hwnd),ctypes.byref(rc))
a=names['a']; b=names['b']
import sys
def shot(tag):
    screen_shot(rc.l,rc.t,rc.r-rc.l,rc.b-rc.t, r'G:\PyArx\docs\_lt_%s.bmp'%tag)
    print(tag, 'chain', zorder(w), 'lv', w.getLevel(a), w.getLevel(b))
shot('q0')
w.setLevel(a,2); p.pump(150); shot('q1')
w.bringToFront(b); p.pump(150); shot('q2')
