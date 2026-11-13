import ctypes, PyMfcXaml as p
from demo_level_tab import build
names={}
w=p.guiload(build(), names=names)
w.show(); p.pump(200)
u=ctypes.windll.user32
u.EnumChildWindows.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_long]
u.GetDlgCtrlID.argtypes=[ctypes.c_void_p]; u.GetDlgCtrlID.restype=ctypes.c_int
u.GetWindowLongPtrW.argtypes=[ctypes.c_void_p,ctypes.c_int]; u.GetWindowLongPtrW.restype=ctypes.c_void_p
u.GetPropW.argtypes=[ctypes.c_void_p,ctypes.c_wchar_p]; u.GetPropW.restype=ctypes.c_void_p
u.IsWindowVisible.argtypes=[ctypes.c_void_p]
u.IsWindowEnabled.argtypes=[ctypes.c_void_p]
CB=ctypes.WINFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_void_p)
@CB
def cb(h,_):
    cid=u.GetDlgCtrlID(ctypes.c_void_p(h))
    st=u.GetWindowLongPtrW(ctypes.c_void_p(h),-16) or 0
    print('id',cid,'style',hex(st),'tabstop',bool(st&0x10000),'vis',bool(u.IsWindowVisible(ctypes.c_void_p(h))),
          'en',bool(u.IsWindowEnabled(ctypes.c_void_p(h))),
          'tabp',u.GetPropW(ctypes.c_void_p(h),'PyMfcTabIndex'),'seqp',u.GetPropW(ctypes.c_void_p(h),'PyMfcSeq'))
    return 1
u.EnumChildWindows(ctypes.c_void_p(w.hwnd),cb,0)
p.pump(200)
