import os, sys, time, ctypes, ctypes.wintypes as wt
os.add_dll_directory(r'G:\PyArx\X64\Release'); sys.path.insert(0, r'G:\PyArx\PyArxQml')
QML = r'G:/PyArx/PyArxQml/qml/power_plan/PowerPlan.qml'
user32=ctypes.windll.user32; kernel32=ctypes.windll.kernel32
LR=ctypes.c_ssize_t; WP=ctypes.WINFUNCTYPE(LR, wt.HWND, wt.UINT, ctypes.c_size_t, LR)
user32.DefWindowProcW.restype=LR; user32.DefWindowProcW.argtypes=[wt.HWND,wt.UINT,ctypes.c_size_t,LR]
proc=WP(lambda h,m,w,l: user32.DefWindowProcW(h,m,w,l))
class WC(ctypes.Structure):
    _fields_=[('cbSize',ctypes.c_uint),('style',ctypes.c_uint),('lpfnWndProc',ctypes.c_void_p),('cbClsExtra',ctypes.c_int),
              ('cbWndExtra',ctypes.c_int),('hInstance',wt.HANDLE),('hIcon',wt.HANDLE),('hCursor',wt.HANDLE),
              ('hbrBackground',wt.HANDLE),('lpszMenuName',wt.LPCWSTR),('lpszClassName',wt.LPCWSTR),('hIconSm',wt.HANDLE)]
hinst=kernel32.GetModuleHandleW(None); wc=WC(); wc.cbSize=ctypes.sizeof(wc)
wc.lpfnWndProc=ctypes.cast(proc,ctypes.c_void_p); wc.hInstance=hinst; wc.lpszClassName='PPDiag'
user32.RegisterClassExW(ctypes.byref(wc))
hwnd=user32.CreateWindowExW(0,'PPDiag','diag',0x00CF0000,100,100,1100,760,None,None,hinst,None)
user32.ShowWindow(hwnd,5)
import PyArxQml
PyArxQml.attach(QML,hwnd,1100,760)
t=time.time()
while time.time()-t<1.2: PyArxQml.pump(); time.sleep(0.006)
print('diag =>', PyArxQml.diag())
print('graphicsApi =>', PyArxQml.eval("(function(){ var w = graph.Window.window; return w ? w.toString() : 'no'; })()"))