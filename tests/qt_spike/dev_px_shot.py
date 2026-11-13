# -*- coding: utf-8 -*-
"""设备像素文字版：抓 40%/100% 图 + 量帧率"""
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
wc.lpszClassName = 'PPDevPx'
user32.RegisterClassExW(ctypes.byref(wc))
hwnd = user32.CreateWindowExW(0, 'PPDevPx', 'devpx', 0x00CF0000, 100, 100, 1100, 760,
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
PyArxQml.eval("(function(){ dir = 'h'; rebuild(); return 1; })()")
pump(0.4)
PyArxQml.eval("(function(){ k = 0.4; tx = 10; ty = 60; return 1; })()")
pump(0.5)
print('grab40', PyArxQml.grab(r'G:\PyArx\docs\_pp_dev_px40.png'))

PyArxQml.eval("(function(){ k = 1.0; tx = 10; ty = 20; return 1; })()")
pump(0.5)
print('grab100', PyArxQml.grab(r'G:\PyArx\docs\_pp_dev_px100.png'))

PyArxQml.eval("(function(){ k = 0.4; tx = 10; ty = 60; return 1; })()")
pump(0.4)
f0 = PyArxQml.stats()['frames']
n = 0
t1 = time.time()
while time.time() - t1 < 2.0:
    PyArxQml.eval("(function(){ tx = tx - 4; return 1; })()")
    PyArxQml.pump()
    time.sleep(0.003)
    n += 1
f1 = PyArxQml.stats()['frames']
print('平移 2 秒：循环 %d 次，交换帧 %d（%.1f fps）' % (n, f1 - f0, (f1 - f0) / 2.0))

f0 = f1
n = 0
t1 = time.time()
while time.time() - t1 < 2.0:
    PyArxQml.eval("(function(){ k = 0.4 + 0.3*((%d) %% 30)/30.0; return 1; })()" % 0)
    PyArxQml.eval("(function(){ k = 0.4 + 0.3*(Math.random()); return 1; })()")
    PyArxQml.pump()
    time.sleep(0.003)
    n += 1
f1 = PyArxQml.stats()['frames']
print('缩放 2 秒：循环 %d 次，交换帧 %d（%.1f fps）' % (n, f1 - f0, (f1 - f0) / 2.0))
