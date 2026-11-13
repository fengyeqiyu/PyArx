# -*- coding: utf-8 -*-
"""直绘版性能实测：每帧耗时（从日志读）+ 连续平移帧率 + 抓图"""
import ctypes
import ctypes.wintypes as wt
import os
import sys
import time

os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\PyArxQml')

QML = r'G:/PyArx/PyArxQml/qml/power_plan/PowerPlan.qml'
OUT = r'G:\PyArx\docs\_pp_ops_zoom40.png'

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
wc.lpszClassName = 'PPOpsPerf'
user32.RegisterClassExW(ctypes.byref(wc))
hwnd = user32.CreateWindowExW(0, 'PPOpsPerf', 'ops', 0x00CF0000, 100, 100, 1100, 760,
                              None, None, hinst, None)
user32.ShowWindow(hwnd, 5)

import PyArxQml
PyArxQml.set_log(r'G:\PyArx\docs\_pyarx_qml.log')
PyArxQml.attach(QML, hwnd, 1100, 760)


def pump(sec):
    t = time.time()
    while time.time() - t < sec:
        PyArxQml.pump()
        time.sleep(0.003)


pump(1.2)
PyArxQml.call('loadThreeLevel')
pump(1.0)
print('info', PyArxQml.call('info'))
print('ops 字节', len(PyArxQml.get_prop('graphOps') or ''))

PyArxQml.eval("(function(){ k = 0.4; tx = 20; ty = 20; return 1; })()")
pump(0.5)
print('grab', PyArxQml.grab(OUT))

# 连续平移 3 秒
f0 = PyArxQml.stats()['frames']
n = 0
t1 = time.time()
while time.time() - t1 < 3.0:
    PyArxQml.eval("(function(){ tx = tx - 4; return 1; })()")
    PyArxQml.pump()
    time.sleep(0.003)
    n += 1
f1 = PyArxQml.stats()['frames']
print('平移 3 秒：脚本循环 %d 次，交换帧 %d（%.1f fps）' % (n, f1 - f0, (f1 - f0) / 3.0))

# 连续缩放 3 秒
f0 = PyArxQml.stats()['frames']
n = 0
k = 0.4
t1 = time.time()
while time.time() - t1 < 3.0:
    k = 0.4 + 0.3 * ((n % 40) / 40.0)
    PyArxQml.eval("(function(){ k = %f; return 1; })()" % k)
    PyArxQml.pump()
    time.sleep(0.003)
    n += 1
f1 = PyArxQml.stats()['frames']
print('缩放 3 秒：脚本循环 %d 次，交换帧 %d（%.1f fps）' % (n, f1 - f0, (f1 - f0) / 3.0))
print('stats', PyArxQml.stats())
