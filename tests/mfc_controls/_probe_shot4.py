import ctypes, PyMfcXaml as p
from demo_level_tab import build
from _verify_level_tab import screen_shot, R, find_by_id, zorder
names={}
w=p.guiload(build(), names=names); w.show()
u=ctypes.windll.user32
for f,a,r in ((u.GetParent,[ctypes.c_void_p],ctypes.c_void_p),
              (u.SetWindowPos,[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_uint],ctypes.c_int),
              (u.GetWindow,[ctypes.c_void_p,ctypes.c_uint],ctypes.c_void_p),
              (u.GetClassNameW,[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int],ctypes.c_int),
              (u.SetForegroundWindow,[ctypes.c_void_p],ctypes.c_int)):
    f.argtypes,f.restype=a,r
u.SetWindowPos(ctypes.c_void_p(w.hwnd),ctypes.c_void_p(-1),60,60,0,0,0x1|0x40); u.SetForegroundWindow(ctypes.c_void_p(w.hwnd)); p.pump(500)
rc=R(); u.GetWindowRect(ctypes.c_void_p(w.hwnd),ctypes.byref(rc))
a=find_by_id(w,names['a']); b=find_by_id(w,names['b'])
for nm,h in (('a',a),('b',b)):
    buf=ctypes.create_unicode_buffer(64); u.GetClassNameW(ctypes.c_void_p(h),buf,64)
    print(nm,'cls',buf.value,'parent',hex(u.GetParent(ctypes.c_void_p(h)) or 0),
          'host?', u.GetParent(ctypes.c_void_p(h))==u.GetWindow(ctypes.c_void_p(w.hwnd),5))
print('z before',zorder(w))
ret=u.SetWindowPos(ctypes.c_void_p(a),ctypes.c_void_p(1),0,0,0,0,0x1|0x2|0x4)
print('setpos ret',ret,'z after',zorder(w))
p.pump(300)
screen_shot(rc.l,rc.t,rc.r-rc.l,rc.b-rc.t,r'G:\PyArx\docs\_lt_s4.bmp')
