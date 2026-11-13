# -*- coding: utf-8 -*-
"""验证 QML 的 Image 是否能渲染 SVG：file:// 与 base64 data URL 两种"""
import base64
import ctypes
import ctypes.wintypes as wt
import os
import sys
import time

os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\PyArxQml')

SVG = ('<svg xmlns="http://www.w3.org/2000/svg" width="200" height="120" viewBox="0 0 200 120">'
       '<rect x="2" y="2" width="196" height="116" fill="#fff" stroke="#2F6FBA" stroke-width="2"/>'
       '<text x="100" y="48" font-family="Microsoft YaHei" font-size="18" fill="#1F2A37" '
       'text-anchor="middle">SVG 渲染测试 中文</text>'
       '<text x="100" y="80" font-family="Microsoft YaHei" font-size="12" fill="#B54708" '
       'text-anchor="middle">220V 单相 · BV-3×4</text></svg>')
TEST_SVG = r'G:\PyArx\docs\_probe.svg'
TEST_QML = r'G:\PyArx\docs\_probe_svg.qml'

with open(TEST_SVG, 'w', encoding='utf-8') as f:
    f.write(SVG)
b64 = base64.b64encode(SVG.encode('utf-8')).decode('ascii')

qml = '''import QtQuick 2.15
Rectangle {
    width: 420; height: 300; color: "#F3F5F8"
    Image { id: i1; x: 10; y: 10; width: 200; height: 120; sourceSize: Qt.size(200,120)
            source: "file:///G:/PyArx/docs/_probe.svg" }
    Image { id: i2; x: 10; y: 150; width: 200; height: 120; sourceSize: Qt.size(200,120)
            source: "data:image/svg+xml;base64,%(b64)s" }
    Text { x: 220; y: 10; text: "file:// status=" + i1.status + " " + i1.errorString; font.pixelSize: 12; color: "#475467" }
    Text { x: 220; y: 150; text: "data status=" + i2.status + " " + i2.errorString; font.pixelSize: 12; color: "#475467" }
}
''' % {'b64': b64}
with open(TEST_QML, 'w', encoding='utf-8') as f:
    f.write(qml)

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
wc.lpszClassName = 'PPSvgHost'
user32.RegisterClassExW(ctypes.byref(wc))
hwnd = user32.CreateWindowExW(0, 'PPSvgHost', 'svg probe', 0x00CF0000, 100, 100, 440, 340,
                              None, None, hinst, None)
user32.ShowWindow(hwnd, 5)

import PyArxQml
print('attach', PyArxQml.attach(TEST_QML, hwnd, 420, 300))
t0 = time.time()
while time.time() - t0 < 1.5:
    PyArxQml.pump()
    time.sleep(0.008)
print('grab', PyArxQml.grab(r'G:\PyArx\docs\_pp_svg_probe.png'))
