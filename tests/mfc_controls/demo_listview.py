# -*- coding: utf-8 -*-
"""第七批演示：虚拟化列表（21 行只画可见段）+ 惯性滚动。窗口保持打开。"""
import os, tempfile
import PyMfcXaml as p

ROWS = ';'.join(['工程 %02d|负责：张%d|%d%%' % (i + 1, (i % 9) + 1, (i * 7) % 100) for i in range(21)])

XAML = '''<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="虚拟化列表演示" Width="480" Height="400">
  <XCanvas>
    <XTextBlock x:Name="lb" Text="VirtualList：21 行，只画可见的 5 行" Width="360" Height="26" Canvas.Left="14" Canvas.Top="8" />
    <XVirtualList x:Name="lv" Items="%s" ItemHeight="44" Width="340" Height="220" Canvas.Left="14" Canvas.Top="44" />
    <XSwitch x:Name="sw" Width="56" Height="28" Canvas.Left="380" Canvas.Top="44" Checked="true" />
  </XCanvas>
</XWindow>''' % ROWS

path = os.path.join(tempfile.mkdtemp(prefix='pymfc_lvdemo_'), 'lv.xaml')
open(path, 'w', encoding='utf-8').write(XAML)
names = {}
w = p.guiload(path, names=names)
w.show()
p.pump(300)
lv = names['lv']
w.setListSelected(lv, 0)
p.pump(120)
print('count=%d sel=%d scroll=%d' % (w.getListItemCount(lv), w.getListSelected(lv),
                                     w.getListScroll(lv)), flush=True)
while True:
    p.pump(50)
