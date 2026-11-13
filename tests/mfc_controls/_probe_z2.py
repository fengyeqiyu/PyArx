import ctypes, time
k=ctypes.windll.kernel32; u=ctypes.windll.user32
u.CreateWindowExW.argtypes=[ctypes.c_uint,ctypes.c_wchar_p,ctypes.c_wchar_p,ctypes.c_uint,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_void_p]
u.CreateWindowExW.restype=ctypes.c_void_p
u.GetWindow.argtypes=[ctypes.c_void_p,ctypes.c_uint]; u.GetWindow.restype=ctypes.c_void_p
u.GetDlgCtrlID.argtypes=[ctypes.c_void_p]; u.GetDlgCtrlID.restype=ctypes.c_int
u.SetWindowPos.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_uint]
WS_OVERLAPPEDWINDOW=0x00CF0000; WS_CHILD=0x40000000; WS_VISIBLE=0x10000000; WS_BORDER=0x800000
top=u.CreateWindowExW(0,'#32770','probe',WS_OVERLAPPEDWINDOW|WS_VISIBLE,100,100,400,300,0,0,0,0)
time.sleep(0.2)
c1=u.CreateWindowExW(0,'Static','one',WS_CHILD|WS_VISIBLE|WS_BORDER,10,10,100,100,top,ctypes.c_void_p(1001),0,0)
c2=u.CreateWindowExW(0,'Static','two',WS_CHILD|WS_VISIBLE|WS_BORDER,40,40,100,100,top,ctypes.c_void_p(1002),0,0)
time.sleep(0.2)
def chain():
    out=[]; c=u.GetWindow(ctypes.c_void_p(top),5)
    while c: out.append(u.GetDlgCtrlID(ctypes.c_void_p(c))); c=u.GetWindow(ctypes.c_void_p(c),2)
    return out
print('before', chain())
print('ret',u.SetWindowPos(ctypes.c_void_p(c1),ctypes.c_void_p(1),0,0,0,0,0x1|0x2|0x3))
print('after', chain())

