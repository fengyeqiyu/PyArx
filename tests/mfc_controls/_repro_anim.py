# -*- coding: utf-8 -*-
"""独立进程复现 cad_form_anim 的窗体（不经 CAD），用于判断 StackView / SpinBox 是否框架自身问题。"""
import os
import ctypes
import PyMfcXaml as p

u = ctypes.windll.user32
u.SetWindowPos.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int,
                           ctypes.c_int, ctypes.c_int, ctypes.c_uint]
u.SetForegroundWindow.argtypes = [ctypes.c_void_p]

XAML = r'G:\PyArx\log\_cad_form_anim.xaml'

names = {}
w = p.guiload(XAML, names=names)
print('names', names)
w.show()
p.pump(300)
u.SetWindowPos(w.hwnd, ctypes.c_void_p(-1), 700, 500, 540, 430, 0x0040)
p.pump(200)

w.animate(names['ab'], 'Width', 300, 600, 'outCubic')
w.animate(names['asp'], 'Value', 90, 800, 'outBack')
try:
    w.animateColor(names['apg'], 0x00A8E8, 700, 'linear')
except Exception as e:
    print('color skipped', e)
w.addPage(names['sv'], [names['pg1']])
w.addPage(names['sv'], [names['pg2']])
w.setViewIndex(names['sv'], 1)
print('pages=%d index=%d' % (w.getViewCount(names['sv']), w.getViewIndex(names['sv'])))

for _ in range(40):
    p.pump(40)          # 让动画跑一会儿
print('done, keep open')
while True:
    p.pump(100)
