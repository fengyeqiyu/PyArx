# -*- coding: utf-8 -*-
"""干净抓图：强制横向布局、固定 40%/100%，并打印布局真实尺寸"""
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
wc.lpszClassName = 'PPShot2'
user32.RegisterClassExW(ctypes.byref(wc))
hwnd = user32.CreateWindowExW(0, 'PPShot2', 'shot2', 0x00CF0000, 100, 100, 1100, 760,
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
pump(0.6)
print('初始 dir =', PyArxQml.eval('dir'), 'layout =', PyArxQml.eval('layout.width'), PyArxQml.eval('layout.height'))

PyArxQml.eval("(function(){ dir = 'h'; rebuild(); fitView(); return 1; })()")
pump(0.6)
print('横向后 dir =', PyArxQml.eval('dir'), 'layout =', PyArxQml.eval('layout.width'), PyArxQml.eval('layout.height'), 'k =', PyArxQml.eval('k'))

# 40%：贴着画布左上角放
PyArxQml.eval("(function(){ k = 0.4; tx = 8; ty = 8; return 1; })()")
pump(0.5)
print('v40 k=', PyArxQml.eval('k'), 'tx=', PyArxQml.eval('tx'), 'info=', PyArxQml.call('info'))
print('grab40', PyArxQml.grab(r'G:\PyArx\docs\_pp_final40.png'))

PyArxQml.eval("(function(){ k = 1.0; tx = 8; ty = 8; return 1; })()")
pump(0.5)
print('grab100', PyArxQml.grab(r'G:\PyArx\docs\_pp_final100.png'))
