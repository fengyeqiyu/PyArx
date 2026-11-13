# -*- coding: utf-8 -*-
"""量 Canvas 版每帧重绘耗时与帧率"""
import ctypes
import ctypes.wintypes as wt
import os
import sys
import time

os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\PyArxQml')

QML = r'G:/PyArx/PyArxQml/qml/power_plan/PowerPlan.qml'

user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32
LR = ctypes.c_ssize_t
WP = ctypes.WINFUNCTYPE(LR, wt.HWND, wt.UINT, ctypes.c_size_t, LR)
user32.DefWindowProcW.restype = LR
user32.DefWindowProcW.argtypes = [wt.HWND, wt.UINT, ctypes.c_size_t, LR]
proc = WP(lambda h, m, w, l: user32.DefWindowProcW(h, m, w, l))


class WC(ctypes.Structure):
    _fields_ = [('cbSize', ctypes.c_uint), ('style', ctypes.c_uint),
                ('lpfnWndProc', ctypes.c_void_p), ('cbClsExtra', ctypes.c_int),
                ('cbWndExtra', ctypes.c_int), ('hInstance', wt.HANDLE),
                ('hIcon', wt.HANDLE), ('hCursor', wt.HANDLE),
                ('hbrBackground', wt.HANDLE), ('lpszMenuName', wt.LPCWSTR),
                ('lpszClassName', wt.LPCWSTR), ('hIconSm', wt.HANDLE)]


hinst = kernel32.GetModuleHandleW(None)
wc = WC()
wc.cbSize = ctypes.sizeof(wc)
wc.lpfnWndProc = ctypes.cast(proc, ctypes.c_void_p)
wc.hInstance = hinst
wc.lpszClassName = 'PPCanvasPerf'
user32.RegisterClassExW(ctypes.byref(wc))
hwnd = user32.CreateWindowExW(0, 'PPCanvasPerf', 'perf', 0x00CF0000, 100, 100, 1100, 760,
                              None, None, hinst, None)
user32.ShowWindow(hwnd, 5)

import PyArxQml
PyArxQml.attach(QML, hwnd, 1100, 760)


def settle(s):
    t = time.time()
    while time.time() - t < s:
        PyArxQml.pump()
        time.sleep(0.006)


settle(1.2)
print('stats', PyArxQml.stats())
print('单次 renderGraph 耗时(ms, 20 次平均)',
      PyArxQml.eval("(function(){ var n=20, t0=performance.now();"
                    " for (var i=0;i<n;i++){ k = 0.2 + i*0.001; renderGraph(); }"
                    " return (performance.now()-t0)/n; })()"))
print('单次 alone 平移重绘(ms)',
      PyArxQml.eval("(function(){ var n=20, t0=performance.now();"
                    " for (var i=0;i<n;i++){ tx += 1; renderGraph(); }"
                    " return (performance.now()-t0)/n; })()"))

# 连续平移 1 秒，看帧数
f0 = PyArxQml.stats()['frames']
t1 = time.time()
n = 0
while time.time() - t1 < 1.0:
    PyArxQml.eval("(function(){ tx -= 6; renderGraph(); return 1; })()")
    PyArxQml.pump()
    n += 1
f1 = PyArxQml.stats()['frames']
print('1 秒内循环 %d 次，交换帧 %d（%.0f fps）' % (n, f1 - f0, (f1 - f0)))
