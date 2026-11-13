# -*- coding: utf-8 -*-
"""探针：attach 后查 QML 内部状态（editRows / layout / 控件树）"""
import ctypes
import ctypes.wintypes as wt
import os
import sys

os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\PyArxQml')

QML = r'G:/PyArx/PyArxQml/qml/power_plan/PowerPlan.qml'

user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32
LR = ctypes.c_ssize_t
WNDPROC = ctypes.WINFUNCTYPE(LR, wt.HWND, wt.UINT, ctypes.c_size_t, LR)
user32.DefWindowProcW.restype = LR
user32.DefWindowProcW.argtypes = [wt.HWND, wt.UINT, ctypes.c_size_t, LR]
proc = WNDPROC(lambda h, m, w, l: user32.DefWindowProcW(h, m, w, l))


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
wc.lpszClassName = 'PPProbeHost'
user32.RegisterClassExW(ctypes.byref(wc))
hwnd = user32.CreateWindowExW(0, 'PPProbeHost', 'probe', 0x00CF0000, 100, 100, 1100, 760,
                              None, None, hinst, None)
user32.ShowWindow(hwnd, 5)

import PyArxQml
print('attach', PyArxQml.attach(QML, hwnd, 1100, 760))
for _ in range(2000):
    PyArxQml.pump()

for expr in ['editRows.length', 'editorOpen', 'editor.width', 'editor.x',
             'toolbar.height', 'toolbarFlow.height', 'toolbarFlow.width',
             'docTitle', 'layout.width', 'layout.height', 'k', 'tx', 'ty']:
    try:
        print('%-22s = %r' % (expr, PyArxQml.eval(expr)))
    except Exception as e:      # noqa: BLE001
        print('%-22s ! %r' % (expr, e))

print('--- dpr/zoom 测试 ---')
PyArxQml.eval("(function(){ zoomAt(400,300,4.5); return 1; })()")
for _ in range(300):
    PyArxQml.pump()
print('k =', PyArxQml.eval('k'))
print('grab100 =', PyArxQml.grab(r'G:\\PyArx\\docs\\_pp_zoom100.png'))
print('--- tree ---')
import json
print(json.dumps(PyArxQml.tree(), ensure_ascii=False, indent=1)[:3000])
print('--- names ---')
print(PyArxQml.names())
