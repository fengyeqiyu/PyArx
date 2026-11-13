# -*- coding: utf-8 -*-
"""配电干线图 QML 版冒烟测试：自建宿主窗口 → attach → pump 到首帧 → 抓图 → 退出"""
import ctypes
import ctypes.wintypes as wt
import os
import sys
import time

os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\PyArxQml')

QML = r'G:/PyArx/PyArxQml/qml/power_plan/PowerPlan.qml'
LOG = r'G:\PyArx\docs\_pyarx_qml_powerplan.log'
PNG = r'G:\PyArx\docs\_pyarx_qml_powerplan.png'

user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32
LR = ctypes.c_ssize_t
WNDPROC = ctypes.WINFUNCTYPE(LR, wt.HWND, wt.UINT, ctypes.c_size_t, LR)
user32.DefWindowProcW.restype = LR
user32.DefWindowProcW.argtypes = [wt.HWND, wt.UINT, ctypes.c_size_t, LR]


def _proc(hwnd, msg, wp, lp):
    return user32.DefWindowProcW(hwnd, msg, wp, lp)


proc = WNDPROC(_proc)


class WNDCLASSEXW(ctypes.Structure):
    _fields_ = [('cbSize', ctypes.c_uint), ('style', ctypes.c_uint),
                ('lpfnWndProc', ctypes.c_void_p), ('cbClsExtra', ctypes.c_int),
                ('cbWndExtra', ctypes.c_int), ('hInstance', wt.HANDLE),
                ('hIcon', wt.HANDLE), ('hCursor', wt.HANDLE),
                ('hbrBackground', wt.HANDLE), ('lpszMenuName', wt.LPCWSTR),
                ('lpszClassName', wt.LPCWSTR), ('hIconSm', wt.HANDLE)]


def make_window(w, h):
    hinst = kernel32.GetModuleHandleW(None)
    cls = 'PowerPlanSmokeHost'
    wc = WNDCLASSEXW()
    wc.cbSize = ctypes.sizeof(wc)
    wc.lpfnWndProc = ctypes.cast(proc, ctypes.c_void_p)
    wc.hInstance = hinst
    wc.lpszClassName = cls
    user32.RegisterClassExW(ctypes.byref(wc))
    hwnd = user32.CreateWindowExW(0, cls, '配电干线图', 0x00CF0000,
                                  200, 100, w, h, None, None, hinst, None)
    user32.ShowWindow(hwnd, 5)
    return hwnd


def mark(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('[smoke] %s\n' % msg)
    print(msg)


mark('=== 开始 %s' % time.strftime('%H:%M:%S'))
hwnd = make_window(1100, 760)

import PyArxQml
mark('import OK')

t = time.perf_counter()
ok = PyArxQml.attach(QML, hwnd, 1100, 760)
mark('attach = %s（%.1f ms）' % (ok, (time.perf_counter() - t) * 1000))
if not ok:
    sys.exit(2)

t = time.perf_counter()
frames = 0
loops = 0
while loops < 4000:
    PyArxQml.pump()
    loops += 1
    frames = PyArxQml.stats().get('frames', 0)
    if frames >= 2:
        break
mark('首帧 %.1f ms（pump %d 次，frames=%d）' % ((time.perf_counter() - t) * 1000, loops, frames))

mark('names = %r' % (PyArxQml.names(),))
try:
    mark('info = %s' % PyArxQml.call('info'))
    mark('exportCsv 前 60 字 = %r' % PyArxQml.call('exportCsvText')[:60])
    mark('eval docTitle = %r' % PyArxQml.eval('docTitle'))
except Exception as e:      # noqa: BLE001
    mark('调用失败：%r' % (e,))

for _ in range(30):
    PyArxQml.pump()
ok = PyArxQml.grab(PNG)
mark('抓图 = %s -> %s' % (ok, PNG))
