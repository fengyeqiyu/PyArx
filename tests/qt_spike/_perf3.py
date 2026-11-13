import ctypes, ctypes.wintypes as wt, os, sys, time
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\PyArxQml')
QML = r'G:/PyArx/PyArxQml/qml/power_plan/PowerPlan.qml'
user32=ctypes.windll.user32; kernel32=ctypes.windll.kernel32
LR=ctypes.c_ssize_t; WP=ctypes.WINFUNCTYPE(LR, wt.HWND, wt.UINT, ctypes.c_size_t, LR)
user32.DefWindowProcW.restype=LR; user32.DefWindowProcW.argtypes=[wt.HWND,wt.UINT,ctypes.c_size_t,LR]
proc=WP(lambda h,m,w,l: user32.DefWindowProcW(h,m,w,l))
class WC(ctypes.Structure):
    _fields_=[('cbSize',ctypes.c_uint),('style',ctypes.c_uint),('lpfnWndProc',ctypes.c_void_p),
              ('cbClsExtra',ctypes.c_int),('cbWndExtra',ctypes.c_int),('hInstance',wt.HANDLE),
              ('hIcon',wt.HANDLE),('hCursor',wt.HANDLE),('hbrBackground',wt.HANDLE),
              ('lpszMenuName',wt.LPCWSTR),('lpszClassName',wt.LPCWSTR),('hIconSm',wt.HANDLE)]
hinst=kernel32.GetModuleHandleW(None); wc=WC(); wc.cbSize=ctypes.sizeof(wc)
wc.lpfnWndProc=ctypes.cast(proc,ctypes.c_void_p); wc.hInstance=hinst; wc.lpszClassName='PPPerf3'
user32.RegisterClassExW(ctypes.byref(wc))
hwnd=user32.CreateWindowExW(0,'PPPerf3','perf',0x00CF0000,100,100,1100,760,None,None,hinst,None)
user32.ShowWindow(hwnd,5)
import PyArxQml
t0=time.time(); PyArxQml.attach(QML,hwnd,1100,760)
while time.time()-t0<1.5:
    PyArxQml.pump(); time.sleep(0.006)
# 紧密循环，不 sleep，量最大帧率
t1=time.time(); n=0
while time.time()-t1<1.0:
    PyArxQml.eval("(function(){ tx += 25; if (tx>200) tx=0; repaint(); return 1; })()")
    PyArxQml.pump(); n+=1
print('tight 循环 %d 次/秒 stats:' % n, PyArxQml.stats())
t2=time.time()
for i in range(5):
    PyArxQml.grab(r'G:\PyArx\docs\_perf_grab.png')
print('单次 grab（含整帧栅格+回读）%.1f ms' % ((time.time()-t2)*1000/5))