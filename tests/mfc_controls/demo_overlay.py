# -*- coding: utf-8 -*-
"""浮层族演示：MenuStrip / Menu / ToolTip / Popup。窗口保持打开，按 Ctrl+C 结束。"""
import ctypes, os, tempfile
from ctypes import wintypes
import PyMfcXaml as p

u = ctypes.windll.user32
u.SetWindowPos.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int,
                           ctypes.c_int, ctypes.c_int, wintypes.UINT]
u.SetForegroundWindow.argtypes = [ctypes.c_void_p]

XAML = '''<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="浮层族演示" Width="700" Height="500">
  <XCanvas>
    <XMenuBar x:Name="mb" Items="文件|编辑|视图|帮助" Width="420" Height="30" Canvas.Left="12" Canvas.Top="12" />
    <XButton x:Name="bt" Width="160" Height="34" Text="悬停看提示" Canvas.Left="12" Canvas.Top="70" />
    <XMenu x:Name="mn" Items="新建|打开|保存|-|退出" Width="160" Height="140" Canvas.Left="210" Canvas.Top="170" />
    <XPopup x:Name="pp" Title="关于 PyArx" Content="版本 1.0|自绘浮层族|Menu · MenuStrip|ToolTip · Popup" Width="300" Height="170" Canvas.Left="380" Canvas.Top="120" />
    <XSwitch x:Name="sw" Width="90" Height="34" Canvas.Left="12" Canvas.Top="360" />
    <XLabel x:Name="tx" Text="浮层全部是宿主内的覆盖子窗口" Width="400" Height="26" Canvas.Left="12" Canvas.Top="420" />
  </XCanvas>
</XWindow>'''

path = os.path.join(tempfile.mkdtemp(prefix='pymfc_overlay_demo_'), 'd.xaml')
open(path, 'w', encoding='utf-8').write(XAML)
names = {}
w = p.guiload(path, names=names)
w.show()
p.pump(300)
u.SetWindowPos(w.hwnd, ctypes.c_void_p(-1), 180, 100, 720, 540, 0x0040)
u.SetForegroundWindow(w.hwnd)
p.pump(400)

w.setMenuBarSelected(names['mb'], 1)
w.showToolTip(names['bt'], '这是一个自绘 ToolTip', 0)
w.showPopup(names['pp'], '关于 PyArx', ['版本 1.0', '自绘浮层族', 'Menu · MenuStrip', 'ToolTip · Popup'],
            380, 120, 300, 170)

while True:
    p.pump(50)
