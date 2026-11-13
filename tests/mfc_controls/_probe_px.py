import ctypes, os, struct
import PyMfcXaml as p
from demo_level_tab import build
from _verify_flickable import capture
names={}
w=p.guiload(build(), names=names)
w.show(); p.pump(300)
u=ctypes.windll.user32
u.GetWindowRect.argtypes=[ctypes.c_void_p,ctypes.c_void_p]
class R(ctypes.Structure): _fields_=[('l',ctypes.c_long),('t',ctypes.c_long),('r',ctypes.c_long),('b',ctypes.c_long)]
def px(path,x,y):
    d=open(path,'rb').read()
    off=struct.unpack_from('<I',d,10)[0]; ww=struct.unpack_from('<i',d,18)[0]
    hh=struct.unpack_from('<i',d,22)[0]; bpp=struct.unpack_from('<H',d,28)[0]
    stride=((ww*bpp//8+3)//4)*4; H=abs(hh)
    row=y if hh<0 else H-1-y
    i=off+row*stride+x*4
    return (d[i+2],d[i+1],d[i])
for nm,fn in (('a','_p_a.bmp'),('b','_p_b.bmp'),('host','_p_h.bmp')):
    hw = w.hwnd if nm=='host' else None
    if nm!='host':
        CB=ctypes.WINFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_void_p)
        found=[]
        @CB
        def cb(h,_):
            if u.GetDlgCtrlID(ctypes.c_void_p(h))==names[nm]: found.append(h); return 0
            return 1
        u.EnumChildWindows.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_long]
        u.EnumChildWindows(ctypes.c_void_p(w.hwnd),cb,0)
        hw=found[0]
    capture(ctypes.c_void_p(hw), fn)
    rc=R(); u.GetWindowRect(ctypes.c_void_p(hw),ctypes.byref(rc))
    print(nm,'rect',rc.l,rc.t,rc.r,rc.b,'size',rc.r-rc.l,rc.b-rc.t,'px(mid)',px(fn,(rc.r-rc.l)//2,(rc.b-rc.t)//2),'px(5,5)',px(fn,5,5))
