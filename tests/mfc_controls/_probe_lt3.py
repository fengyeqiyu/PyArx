import ctypes, PyMfcXaml as p
from demo_level_tab import build
names={}
w=p.guiload(build(), names=names)
w.show(); p.pump(300)
u=ctypes.windll.user32
u.GetParent.argtypes=[ctypes.c_void_p]; u.GetParent.restype=ctypes.c_void_p
u.GetDlgCtrlID.argtypes=[ctypes.c_void_p]; u.GetDlgCtrlID.restype=ctypes.c_int
u.SetFocus.argtypes=[ctypes.c_void_p]; u.SetFocus.restype=ctypes.c_void_p
u.GetFocus.restype=ctypes.c_void_p
u.GetPropW.argtypes=[ctypes.c_void_p,ctypes.c_wchar_p]; u.GetPropW.restype=ctypes.c_void_p
u.GetWindowLongPtrW.argtypes=[ctypes.c_void_p,ctypes.c_int]; u.GetWindowLongPtrW.restype=ctypes.c_void_p
u.PostMessageW.argtypes=[ctypes.c_void_p,ctypes.c_uint,ctypes.c_void_p,ctypes.c_void_p]
def find(cid):
    got=[]
    CB=ctypes.WINFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_void_p)
    @CB
    def cb(h,_):
        if u.GetDlgCtrlID(ctypes.c_void_p(h))==cid: got.append(h); return 0
        return 1
    u.EnumChildWindows(ctypes.c_void_p(w.hwnd),cb,0)
    return got[0] if got else None
for nm in ('e1','e2','e3'):
    h=find(names[nm])
    style=u.GetWindowLongPtrW(ctypes.c_void_p(h),-16)
    par=u.GetParent(ctypes.c_void_p(h))
    print(nm, 'id',names[nm],'hwnd',hex(h),'style',hex(style or 0),
          'tabstop',bool((style or 0)&0x10000),'tabs',u.GetPropW(ctypes.c_void_p(h),'PyMfcTabIndex'),
          'seq',u.GetPropW(ctypes.c_void_p(h),'PyMfcSeq'),
          'parent tabhost',u.GetPropW(ctypes.c_void_p(par),'PyMfcTabHost'))
e1=find(names['e1'])
u.SetFocus(ctypes.c_void_p(e1)); p.pump(60)
print('focus1', u.GetDlgCtrlID(ctypes.c_void_p(u.GetFocus())))
u.PostMessageW(ctypes.c_void_p(e1),0x0100,ctypes.c_void_p(9),ctypes.c_void_p(0))
for i in range(6):
    p.pump(40)
    print(' after',i,u.GetDlgCtrlID(ctypes.c_void_p(u.GetFocus())))
