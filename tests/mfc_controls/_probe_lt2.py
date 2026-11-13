import ctypes, PyMfcXaml as p
from demo_level_tab import build
names={}
w=p.guiload(build(), names=names)
w.show(); p.pump(300)
u=ctypes.windll.user32
u.GetWindow.argtypes=[ctypes.c_void_p,ctypes.c_uint]; u.GetWindow.restype=ctypes.c_void_p
u.GetDlgCtrlID.argtypes=[ctypes.c_void_p]; u.GetDlgCtrlID.restype=ctypes.c_int
u.SetFocus.argtypes=[ctypes.c_void_p]; u.GetFocus.restype=ctypes.c_void_p
u.GetPropW.argtypes=[ctypes.c_void_p,ctypes.c_wchar_p]; u.GetPropW.restype=ctypes.c_void_p
u.keybd_event.argtypes=[ctypes.c_byte,ctypes.c_byte,ctypes.c_int,ctypes.c_void_p]
def find(cid):
    got=[]
    CB=ctypes.WINFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_void_p)
    @CB
    def cb(h,_):
        if u.GetDlgCtrlID(ctypes.c_void_p(h))==cid: got.append(h); return 0
        return 1
    u.EnumChildWindows(ctypes.c_void_p(w.hwnd),cb,0)
    return got[0] if got else None
e1=find(names['e1']); print("e1 hwnd", hex(e1))
cur=ctypes.c_void_p(e1); 
for _ in range(5):
    print("  prop on", hex(cur.value or 0), u.GetPropW(cur,'PyMfcTabHost') if cur else None)
    cur=ctypes.c_void_p(u.GetWindow(cur,4)) if cur else None
    if not cur: break
u.SetFocus(ctypes.c_void_p(e1)); p.pump(60)
print("focus before", u.GetDlgCtrlID(ctypes.c_void_p(u.GetFocus())))
u.keybd_event(9,0,0,None); u.keybd_event(9,0,2,None)
p.pump(120)
print("focus after real tab", u.GetDlgCtrlID(ctypes.c_void_p(u.GetFocus())))
