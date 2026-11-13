# -*- coding: utf-8 -*-
"""矢量图元性能：首帧、连续缩放、连续平移的帧率"""
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
wc.lpszClassName = 'PPPerfHost'
user32.RegisterClassExW(ctypes.byref(wc))
hwnd = user32.CreateWindowExW(0, 'PPPerfHost', 'perf', 0x00CF0000, 100, 100, 1100, 760,
                              None, None, hinst, None)
user32.ShowWindow(hwnd, 5)

t0 = time.time()
import PyArxQml
PyArxQml.attach(QML, hwnd, 1100, 760)
first = None
while time.time() - t0 < 1.5:
    PyArxQml.pump()
    if first is None:
        first = time.time() - t0
print('attach+首帧 %.1f ms' % ((first or 0) * 1000))


def settle(s):
    t1 = time.time()
    while time.time() - t1 < s:
        PyArxQml.pump()
        time.sleep(0.006)


settle(0.6)

# 连续缩放 10 次（每次重建 SVG + 重绘）
tA = time.time()
for i in range(10):
    PyArxQml.eval("(function(){ zoomAt(500,300, 1.2); return 1; })()")
    PyArxQml.pump()
print('缩放 10 次耗时 %.1f ms（含重建 SVG）' % ((time.time() - tA) * 1000))

# 连续平移 20 步（只改 offset，不重建 SVG）
tB = time.time()
for i in range(20):
    PyArxQml.eval("(function(){ tx += 40; repaint(); return 1; })()")
    PyArxQml.pump()
print('平移 20 步耗时 %.1f ms（每步 %.1f ms）' % ((time.time() - tB) * 1000, (time.time() - tB) * 50))

# 单次重建 SVG 的纯脚本耗时
tC = time.time()
n = 20
for i in range(n):
    PyArxQml.eval("(function(){ refreshSvg(); return graphSvg.length; })()")
print('refreshSvg 单次 %.2f ms（SVG 文本 %s 字符）'
      % ((time.time() - tC) * 1000 / n, PyArxQml.eval('graphSvg.length')))
