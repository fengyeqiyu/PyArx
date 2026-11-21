# -*- coding: utf-8 -*-
"""第九批演示：Repeater / GridView / PathView / QCanvas 同窗展示，窗口保持打开。"""
import os, sys, tempfile
import PyMfcXml as p

CMDS = ('Clear #2B313C; Rect 24,24,150,80,#00A8E8; RoundRect 24,130,150,60,12,#E8A800; '
        'Ellipse 210,24,90,90,#57C84D; Line 24,215,580,215,#FFFFFF,2; '
        'Text 210,150,画布由命令串驱动,13,#E6EDF3')

XML = '''<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xml" Title="第九批 Repeater / GridView / PathView / QCanvas" Width="1240" Height="900">
  <XCanvas>
    <Repeater x:Name="rep" Items="甲|乙|丙|丁" ItemHeight="34" Spacing="0" Canvas.Left="12" Canvas.Top="44">
      <XButton x:Name="rb" Text="行{item}" Width="104" Height="30" />
    </Repeater>
    <XGridView x:Name="gv" Width="300" Height="320" CellHeight="74" Canvas.Left="130" Canvas.Top="44" />
    <XPathView x:Name="pv" Width="304" Height="150" Canvas.Left="444" Canvas.Top="44" />
    <XQCanvas x:Name="qc" Width="618" Height="240" Canvas.Left="130" Canvas.Top="384" Commands="%s" />
    <XLabel x:Name="t" Text="Repeater 展开按钮 / GridView 12 格 / PathView 6 卡 / QCanvas 画命令" Width="700" Height="22" Canvas.Left="14" Canvas.Top="8" />
  </XCanvas>
</XWindow>''' % CMDS

path = os.path.join(tempfile.mkdtemp(prefix='pymfc_qml9_'), 'q9.xml')
open(path, 'w', encoding='utf-8').write(XML)
names = {}
w = p.guiload(path, names=names)
w.show()
p.pump(600)
import ctypes
from ctypes import wintypes
for _ in range(3):
    ctypes.windll.user32.MoveWindow(w.hwnd, 300, 90, 980, 760, True)
    p.pump(300)
r = wintypes.RECT()
ctypes.windll.user32.GetWindowRect(w.hwnd, ctypes.byref(r))
print('rect', r.left, r.top, r.right - r.left, r.bottom - r.top, flush=True)
gv, pv, qc = names['gv'], names['pv'], names['qc']
w.set_canvas_commands(qc, CMDS)

titles = ['工程 %02d' % i for i in range(1, 13)]
subs = ['动力源', '配电', '照明', '桥架', '接地', '通风']
for i, t in enumerate(titles):
    w.add_list_item(gv, '%s|%s|%d' % (t, subs[i % len(subs)], (i + 1) * 4))
w.set_list_selected(gv, 3)

for i, t in enumerate(['封面', '总图', '配电室', '动力源', '桥架', '详图']):
    w.add_list_item(pv, '%s|第 %d 张' % (t, i + 1))
w.set_list_selected(pv, 2)
p.pump(400)
print('grid=%d path=%d canvas=%d sel=%d/%d' % (w.get_list_item_count(gv), w.get_list_item_count(pv), qc, w.get_list_selected(gv), w.get_list_selected(pv)), flush=True)
while True:
    p.pump(50)
