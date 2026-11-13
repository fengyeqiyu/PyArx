# -*- coding: utf-8 -*-
"""动画演示：PropertyAnimation / NumberAnimation / ColorAnimation，往复循环，窗口保持打开。"""
import ctypes, os, tempfile, time
from ctypes import wintypes
import PyMfcXaml as p

u = ctypes.windll.user32
u.SetWindowPos.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int,
                           ctypes.c_int, ctypes.c_int, wintypes.UINT]
u.SetForegroundWindow.argtypes = [ctypes.c_void_p]

XAML = '''<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="动画引擎演示" Width="560" Height="380">
  <XCanvas>
    <XLabel x:Name="t1" Text="PropertyAnimation：按钮 Width 140 → 420 往复（linear）" Width="460" Height="26" Canvas.Left="24" Canvas.Top="18" />
    <XButton x:Name="bt" Width="140" Height="44" Text="宽度动画" Canvas.Left="24" Canvas.Top="52" />
    <XLabel x:Name="t2" Text="NumberAnimation：SpinBox Value 0 → 99 往复（outCubic）" Width="460" Height="26" Canvas.Left="24" Canvas.Top="120" />
    <XSpinBox x:Name="sp" Width="220" Height="34" Value="0" Minimum="0" Maximum="99" Canvas.Left="24" Canvas.Top="152" />
    <XLabel x:Name="t3" Text="ColorAnimation：Switch 底色在主题灰与强调蓝之间往复" Width="460" Height="26" Canvas.Left="24" Canvas.Top="206" />
    <XSwitch x:Name="sw" Width="120" Height="45" Value="1" Canvas.Left="24" Canvas.Top="238" />
  </XCanvas>
</XWindow>'''

path = os.path.join(tempfile.mkdtemp(prefix='pymfc_anim_demo_'), 'd.xaml')
open(path, 'w', encoding='utf-8').write(XAML)
names = {}
w = p.guiload(path, names=names)
w.show()
p.pump(300)
u.SetWindowPos(w.hwnd, ctypes.c_void_p(-1), 780, 700, 580, 430, 0x0040)
u.SetForegroundWindow(w.hwnd)
p.pump(300)

bt, sp, sw = names['bt'], names['sp'], names['sw']
CYC = 10800          # 一整轮 ping-pong 的时长（ms）


def schedule():
    # 12 段 900ms：140 → 420 → 140 …（SequentialAnimation 的等价写法：t0 依次累加）
    for i in range(12):
        a, b = (140, 420) if i % 2 == 0 else (420, 140)
        w.animateFrom(bt, 'Width', a, b, 900, 'linear', i * 900, False)
    # NumberAnimation：6 个来回，每半程 750ms
    for i in range(12):
        a, b = (0, 99) if i % 2 == 0 else (99, 0)
        w.animateFrom(sp, 'Value', a, b, 750, 'outCubic', i * 900, False)
    # ColorAnimation：3 个来回，每半程 1800ms
    for i in range(6):
        c = 0x00D27800 if i % 2 == 0 else 0x00313832
        w.animateColor(sw, c, 1800, 'outCubic', i * 1800)


schedule()
tick = time.time()
while True:
    p.pump(50)
    if time.time() - tick > (CYC - 300) / 1000.0:
        schedule()
        tick = time.time()
